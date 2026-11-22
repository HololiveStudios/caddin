#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif
#include "neuro.h"
#include "network_data.h"
#include "src/core/persist.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define NS (PESTINHA_CHANNELS * PESTINHA_SENSORY_PER_POOL)
#define NL (PESTINHA_NEURONS - NS)
#define NPOOLS (PESTINHA_CHANNELS + PESTINHA_INTERNEURONS + PESTINHA_MOTORS)
#define NDEFAULT_CONNS ((int)(sizeof(DEFAULT_CONNS) / sizeof(DEFAULT_CONNS[0])))
#define MAX_CONN_RULES (sizeof(DEFAULT_CONNS) / sizeof(DEFAULT_CONNS[0]) + PESTINHA_INTERNEURONS)

#define LEGACY_NEURONS 2048u
#define LEGACY_SENSORY 512u
#define LEGACY_ASSOCIATIVE 1152u
#define LEGACY_MOTOR 384u
#define LEGACY_FREE (LEGACY_NEURONS - LEGACY_SENSORY)
#define MAX_OUT_DEGREE 64u
#define MAX_CACHED_GAP 400u

#define STDP_TAU_POS 20.0  /* ms */
#define STDP_TAU_NEG 20.0  /* ms */
#define STDP_A_POS   0.005
#define STDP_A_NEG   0.0045
#define STDP_W_MIN  -0.5f
#define STDP_W_MAX   1.0f

typedef struct {
    int post_pool;
    double weight;
    double density;
} ConnRule;

struct Brain {
    unsigned int rng;

    int starts[NPOOLS];
    int sizes[NPOOLS];
    int kind[NPOOLS];
    const char *names[NPOOLS];
    uint8_t *pool_of_neuron;
    ConnRule conn_rules[NPOOLS][MAX_CONN_RULES];
    uint16_t conn_rule_count[NPOOLS];
    double passive_decay_power[NPOOLS][MAX_CACHED_GAP + 1u];
    double passive_adapt_power[NPOOLS][MAX_CACHED_GAP + 1u];
    double passive_kernel[NPOOLS][MAX_CACHED_GAP + 1u];
    double stdp_ltp_factor[50];
    double stdp_ltd_factor[50];

    uint32_t *row_ptr;                  /* CSR: início das sinapses de cada neurônio pré */
    uint32_t *post;                      /* Índices pós-sinápticos */
    float *w;                            /* Pesos plásticos, armazenados apenas para sinapses existentes */
    int8_t *w0;                          /* Peso basal quantizado por sinapse */
    uint32_t n_synapses;

    double *v;                       /* estado neural em heap (SoA) */
    double *adapt;
    double *adapt_b;
    double *adapt_decay;
    double *decay;
    int *refractory;
    double *drive;
    uint64_t *drive_stamp;
    uint64_t *last_update_tick;

    uint32_t *fired_ids;
    uint32_t *next_fired_ids;
    uint32_t fired_count;
    uint32_t next_fired_count;
    uint32_t *active_targets;
    uint32_t active_target_count;
    uint8_t *plastic_mark;
    double *last_spike_t;
    double *pre_gain;

    double *sensory_rate;
    double *sensory_d;
    double *sensory_rec;
    double *efficacy;
    double *probability;
    uint64_t *sensory_update_tick;

    double levels[PESTINHA_CHANNELS];
    double pool_counts[NPOOLS];
    double pool_rates[NPOOLS];

    int active_spikes_frame;
    uint32_t *plastic_pre;
    uint32_t plastic_pre_count;
    double total_plastic_delta;
    double time_ms;
    uint64_t tick;
    BrainStepStats stats;
};

static unsigned int rand_u(unsigned int *s) {
    *s ^= *s << 13;
    *s ^= *s >> 17;
    *s ^= *s << 5;
    return *s;
}

static double urand(Brain *b) {
    return ((rand_u(&b->rng) >> 8) + 1.0) / 16777217.0;
}

static uint64_t monotonic_ns(void) {
#ifdef _WIN32
    LARGE_INTEGER counter, frequency;
    QueryPerformanceCounter(&counter);
    QueryPerformanceFrequency(&frequency);
    return (uint64_t)((counter.QuadPart * 1000000000ULL) / frequency.QuadPart);
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
}

static int find_pool(const Brain *b, const char *n) {
    for (int i = 0; i < NPOOLS; ++i) {
        if (b->names[i] && b->names[i][0] == n[0] && !strcmp(n, b->names[i])) return i;
    }
    return -1;
}

static int pre_pool_for_index(const Brain *b, uint32_t index) {
    return (b && index < PESTINHA_NEURONS) ? b->pool_of_neuron[index] : -1;
}

static int brain_validate_connectome_defs(const Brain *b) {
    if (!b) return 0;
    for (int i = 0; i < NPOOLS; ++i) {
        if (!b->names[i] || b->sizes[i] <= 0) return 0;
        for (int j = i + 1; j < NPOOLS; ++j) {
            if (strcmp(b->names[i], b->names[j]) == 0) return 0;
        }
    }
    const int n = (int)(sizeof(DEFAULT_CONNS) / sizeof(DEFAULT_CONNS[0]));
    for (int i = 0; i < n; ++i) {
        const ConnDef *c = &DEFAULT_CONNS[i];
        if (find_pool(b, c->pre) < 0 || find_pool(b, c->post) < 0 ||
            !isfinite(c->weight) || !isfinite(c->density) ||
            c->weight == 0.0 || fabs(c->weight) > STDP_W_MAX ||
            c->density <= 0.0 || c->density > 1.0) return 0;
    }
    return 1;
}

const char *brain_channel_name(int i) {
    return (i >= 0 && i < PESTINHA_CHANNELS) ? SENS_POOLS[i].name : "";
}

const char *brain_motor_name(int i) {
    return (i >= 0 && i < PESTINHA_MOTORS) ? MOTOR_POOLS[i].name : "";
}

const char *brain_interneuron_name(int i) {
    return (i >= 0 && i < PESTINHA_INTERNEURONS) ? INTER_POOLS[i].name : "";
}

static int edge_exists(const uint32_t *posts, uint32_t count, uint32_t target) {
    for (uint32_t i = 0; i < count; ++i) if (posts[i] == target) return (int)i;
    return -1;
}

static void add_edge(uint32_t *posts, float *weights, int8_t *bases,
                     uint32_t *count, uint32_t target, double weight) {
    int found = edge_exists(posts, *count, target);
    uint32_t at;
    if (found >= 0) at = (uint32_t)found;
    else {
        if (*count >= MAX_OUT_DEGREE) return;
        at = (*count)++;
        posts[at] = target;
    }
    weight /= (double)PESTINHA_NEURON_SCALE;
    if (weight > STDP_W_MAX) weight = STDP_W_MAX;
    if (weight < STDP_W_MIN) weight = STDP_W_MIN;
    weights[at] = (float)weight;
    int q = (int)round(weight * 127.0);
    if (q > 127) q = 127;
    if (q < -128) q = -128;
    bases[at] = (int8_t)q;
}

static void build_row(const Brain *b, uint32_t pre, unsigned int *rng, uint32_t *posts, float *weights,
                      int8_t *bases, uint32_t *count) {
    *count = 0;
    int pre_pool = pre_pool_for_index(b, pre);
    if (pre_pool < 0) return;

    uint32_t rules = b->conn_rule_count[pre_pool];
    if (!rules) return;

    uint32_t per_rule = MAX_OUT_DEGREE / rules;
    uint32_t remainder = MAX_OUT_DEGREE % rules;
    uint32_t rule_no = 0;
    for (uint32_t i = 0; i < rules; ++i) {
        const ConnRule *rule = &b->conn_rules[pre_pool][i];
        int post_pool = rule->post_pool;
        uint32_t budget = per_rule + (rule_no++ < remainder ? 1u : 0u);
        uint32_t target_count = (uint32_t)b->sizes[post_pool];
        uint32_t wanted = (uint32_t)ceil(target_count * rule->density);
        if (wanted > budget) wanted = budget;
        for (uint32_t k = 0; k < wanted; ++k) {
            uint32_t target;
            uint32_t tries = 0;
            do {
                target = (uint32_t)b->starts[post_pool] + (target_count ? rand_u(rng) % target_count : 0u);
                tries++;
            } while (edge_exists(posts, *count, target) >= 0 && tries < target_count + 1u);
            if (edge_exists(posts, *count, target) < 0)
                add_edge(posts, weights, bases, count, target, rule->weight);
        }
    }
}

static int brain_prepare_conn_rules(Brain *b) {
    if (!b) return 0;
    for (int i = 0; i < NDEFAULT_CONNS; ++i) {
        const ConnDef *c = &DEFAULT_CONNS[i];
        int pre_pool = find_pool(b, c->pre), post_pool = find_pool(b, c->post);
        if (pre_pool < 0 || post_pool < 0) return 0;
        uint16_t at = b->conn_rule_count[pre_pool];
        if (at >= MAX_CONN_RULES) return 0;
        b->conn_rules[pre_pool][at] = (ConnRule){post_pool, c->weight, c->density};
        b->conn_rule_count[pre_pool]++;
    }
    for (int pre_pool = PESTINHA_CHANNELS; pre_pool < PESTINHA_CHANNELS + PESTINHA_INTERNEURONS; ++pre_pool) {
        for (int post_pool = PESTINHA_CHANNELS; post_pool < PESTINHA_CHANNELS + PESTINHA_INTERNEURONS; ++post_pool) {
            if (post_pool == pre_pool) continue;
            uint16_t at = b->conn_rule_count[pre_pool];
            if (at >= MAX_CONN_RULES) return 0;
            b->conn_rules[pre_pool][at] = (ConnRule){post_pool, 0.15, 0.12};
            b->conn_rule_count[pre_pool]++;
        }
    }
    return 1;
}

static void brain_prepare_passive_tables(Brain *b) {
    for (int p = PESTINHA_CHANNELS; p < NPOOLS; ++p) {
        uint32_t j = (uint32_t)b->starts[p] - NS;
        double d = b->decay[j], a = b->adapt_decay[j];
        for (uint32_t gap = 0; gap <= MAX_CACHED_GAP; ++gap) {
            double dp = pow(d, (double)gap), ap = pow(a, (double)gap);
            b->passive_decay_power[p][gap] = dp;
            b->passive_adapt_power[p][gap] = ap;
            b->passive_kernel[p][gap] = gap == 0 ? 0.0 :
                (fabs(d - a) < 1e-12 ? (double)gap * pow(d, (double)(gap - 1u)) : (dp - ap) / (d - a));
        }
    }
}

static void brain_prepare_stdp_tables(Brain *b) {
    for (int ms = 1; ms < 50; ++ms) {
        b->stdp_ltp_factor[ms] = exp(-(double)ms / STDP_TAU_POS);
        b->stdp_ltd_factor[ms] = exp(-(double)ms / STDP_TAU_NEG);
    }
}

static void brain_release_connectome(Brain *b) {
    if (!b) return;
    free(b->row_ptr);
    free(b->post);
    free(b->w);
    free(b->w0);
    b->row_ptr = NULL;
    b->post = NULL;
    b->w = NULL;
    b->w0 = NULL;
    b->n_synapses = 0;
}

/* Cria CSR com fan-out limitado; construção e memória escalam com N*k, não N². */
static int brain_build_connectome(Brain *b) {
    if (!b) return 0;
    const size_t scratch_n = MAX_OUT_DEGREE;
    uint32_t *row_ptr = calloc((size_t)PESTINHA_NEURONS + 1u, sizeof(*row_ptr));
    uint32_t *tmp_posts = malloc(scratch_n * sizeof(*tmp_posts));
    float *tmp_weights = malloc(scratch_n * sizeof(*tmp_weights));
    int8_t *tmp_bases = malloc(scratch_n * sizeof(*tmp_bases));
    if (!row_ptr || !tmp_posts || !tmp_weights || !tmp_bases) {
        free(row_ptr); free(tmp_posts); free(tmp_weights); free(tmp_bases); return 0;
    }
    uint64_t edges = 0;
    uint32_t topology_seed = b->rng ? b->rng : 1u;
    for (uint32_t pre = 0; pre < PESTINHA_NEURONS; ++pre) {
        unsigned int row_rng = topology_seed ^ (pre + 0x9e3779b9u);
        if (!row_rng) row_rng = pre + 1u;
        uint32_t count;
        build_row(b, pre, &row_rng, tmp_posts, tmp_weights, tmp_bases, &count);
        edges += count;
        if (edges > UINT32_MAX) { free(row_ptr); free(tmp_posts); free(tmp_weights); free(tmp_bases); return 0; }
        row_ptr[pre + 1u] = (uint32_t)edges;
    }
    uint32_t *post = malloc((size_t)edges * sizeof(*post));
    float *weights = malloc((size_t)edges * sizeof(*weights));
    int8_t *base = malloc((size_t)edges * sizeof(*base));
    if (edges && (!post || !weights || !base)) {
        free(row_ptr); free(post); free(weights); free(base);
        free(tmp_posts); free(tmp_weights); free(tmp_bases); return 0;
    }
    for (uint32_t pre = 0; pre < PESTINHA_NEURONS; ++pre) {
        unsigned int row_rng = topology_seed ^ (pre + 0x9e3779b9u);
        if (!row_rng) row_rng = pre + 1u;
        uint32_t count;
        build_row(b, pre, &row_rng, tmp_posts, tmp_weights, tmp_bases, &count);
        uint32_t cursor = row_ptr[pre];
        memcpy(post + cursor, tmp_posts, count * sizeof(*post));
        memcpy(weights + cursor, tmp_weights, count * sizeof(*weights));
        memcpy(base + cursor, tmp_bases, count * sizeof(*base));
    }
    free(tmp_posts); free(tmp_weights); free(tmp_bases);
    b->rng = topology_seed;
    brain_release_connectome(b);
    b->row_ptr = row_ptr;
    b->post = post;
    b->w = weights;
    b->w0 = base;
    b->n_synapses = (uint32_t)edges;
    return 1;
}

Brain *brain_create(unsigned int seed, const char *temperament) {
    Brain *b = calloc(1, sizeof(*b));
    uint32_t cursor = 0;
    (void)temperament;
    if (!b) return NULL;

    b->rng = seed ? seed : 42u;
#define ALLOC_FIELD(field, count) do { b->field = calloc((size_t)(count), sizeof(*b->field)); if (!b->field) { brain_destroy(b); return NULL; } } while (0)
    ALLOC_FIELD(v, NL); ALLOC_FIELD(adapt, NL); ALLOC_FIELD(adapt_b, NL);
    ALLOC_FIELD(adapt_decay, NL); ALLOC_FIELD(decay, NL); ALLOC_FIELD(refractory, NL);
    ALLOC_FIELD(drive, NL); ALLOC_FIELD(drive_stamp, NL); ALLOC_FIELD(last_update_tick, NL);
    ALLOC_FIELD(fired_ids, PESTINHA_NEURONS); ALLOC_FIELD(next_fired_ids, PESTINHA_NEURONS);
    ALLOC_FIELD(active_targets, NL); ALLOC_FIELD(pool_of_neuron, PESTINHA_NEURONS);
    ALLOC_FIELD(plastic_mark, PESTINHA_NEURONS);
    ALLOC_FIELD(last_spike_t, PESTINHA_NEURONS); ALLOC_FIELD(pre_gain, PESTINHA_NEURONS);
    ALLOC_FIELD(sensory_rate, NS); ALLOC_FIELD(sensory_d, NS); ALLOC_FIELD(sensory_rec, NS);
    ALLOC_FIELD(efficacy, NS); ALLOC_FIELD(probability, NS);
    ALLOC_FIELD(sensory_update_tick, NS);
    ALLOC_FIELD(plastic_pre, PESTINHA_NEURONS);
#undef ALLOC_FIELD

    /* 1. Canais sensoriais: escala aplicada uniformemente aos 64 grupos. */
    for (int i = 0; i < PESTINHA_CHANNELS; ++i) {
        uint32_t n = (uint32_t)SENS_POOLS[i].n * PESTINHA_NEURON_SCALE;
        b->starts[i] = cursor;
        b->sizes[i]  = n;
        b->kind[i]   = BRAIN_POOL_SENSORY;
        b->names[i]  = SENS_POOLS[i].name;
        cursor += n;
        for (uint32_t j = 0; j < n; ++j) {
            uint32_t q = (uint32_t)b->starts[i] + (uint32_t)j;
            b->pool_of_neuron[q] = (uint8_t)i;
            b->sensory_rate[q] = SENS_POOLS[i].rate;
            b->sensory_d[q]    = SENS_POOLS[i].d;
            b->sensory_rec[q]  = 0.001 / SENS_POOLS[i].tau_rec;
            b->efficacy[q]     = 1.0;
        }
    }

    /* 2. Grupos associativos: excitação/inibição segue a tabela de perfis. */
    for (int i = 0; i < PESTINHA_INTERNEURONS; ++i) {
        int p = PESTINHA_CHANNELS + i;
        b->starts[p] = cursor;
        b->sizes[p]  = INTER_POOLS[i].n * PESTINHA_NEURON_SCALE;
        b->kind[p]   = !strcmp(INTER_POOLS[i].kind, "inh") ? BRAIN_POOL_INHIBITORY : BRAIN_POOL_EXCITATORY;
        b->names[p]  = INTER_POOLS[i].name;
        cursor += b->sizes[p];

        for (uint32_t j = 0; j < (uint32_t)b->sizes[p]; ++j) {
            int q = b->starts[p] + j - NS;
            b->pool_of_neuron[(uint32_t)b->starts[p] + j] = (uint8_t)p;
            b->adapt_b[q]     = INTER_POOLS[i].b;
            b->adapt_decay[q] = exp(-0.001 / INTER_POOLS[i].bt);
            b->decay[q]       = exp(-1.0 / INTER_POOLS[i].tau);
        }
    }

    /* 3. Grupos motores: preservam cada saída existente na escala-alvo. */
    for (int i = 0; i < PESTINHA_MOTORS; ++i) {
        int p = PESTINHA_CHANNELS + PESTINHA_INTERNEURONS + i;
        b->starts[p] = cursor;
        b->sizes[p]  = MOTOR_POOLS[i].n * PESTINHA_NEURON_SCALE;
        b->kind[p]   = BRAIN_POOL_MOTOR;
        b->names[p]  = MOTOR_POOLS[i].name;
        cursor += b->sizes[p];

        for (uint32_t j = 0; j < (uint32_t)b->sizes[p]; ++j) {
            int q = b->starts[p] + j - NS;
            b->pool_of_neuron[(uint32_t)b->starts[p] + j] = (uint8_t)p;
            b->adapt_b[q]     = MOTOR_POOLS[i].b;
            b->adapt_decay[q] = exp(-0.001 / MOTOR_POOLS[i].bt);
            b->decay[q]       = exp(-1.0 / MOTOR_POOLS[i].tau);
        }
    }

    if (cursor != PESTINHA_NEURONS) {
        brain_destroy(b);
        return NULL;
    }

    for (uint32_t i = 0; i < PESTINHA_NEURONS; ++i) {
        b->pre_gain[i] = 1.0;
        b->last_spike_t[i] = -1000.0;
    }

    /* Rejeita grupos duplicados, vazios e conexões mal configuradas. */
    if (!brain_validate_connectome_defs(b)) {
        brain_destroy(b);
        return NULL;
    }

    if (!brain_prepare_conn_rules(b)) {
        brain_destroy(b);
        return NULL;
    }
    brain_prepare_passive_tables(b);
    brain_prepare_stdp_tables(b);

    /* Montagem direta CSR, sem matriz N×NL. */
    if (!brain_build_connectome(b)) {
        brain_destroy(b);
        return NULL;
    }

    return b;
}

void brain_destroy(Brain *b) {
    if (!b) return;
    brain_release_connectome(b);
    free(b->v); free(b->adapt); free(b->adapt_b); free(b->adapt_decay);
    free(b->decay); free(b->refractory); free(b->drive); free(b->drive_stamp);
    free(b->last_update_tick); free(b->fired_ids); free(b->next_fired_ids);
    free(b->active_targets); free(b->pool_of_neuron);
    free(b->plastic_mark);
    free(b->last_spike_t); free(b->pre_gain); free(b->sensory_rate);
    free(b->sensory_d); free(b->sensory_rec); free(b->efficacy); free(b->probability);
    free(b->sensory_update_tick);
    free(b->plastic_pre);
    free(b);
}

void brain_stimulate(Brain *b, const double values[PESTINHA_CHANNELS]) {
    if (!b || !values) return;
    for (int c = 0; c < PESTINHA_CHANNELS; ++c) {
        double x = fmax(0.0, fmin(1.0, values[c]));
        b->levels[c] = x;
        if (x <= 0.0) continue;
        uint32_t start = (uint32_t)b->starts[c];
        uint32_t end = start + (uint32_t)b->sizes[c];
        for (uint32_t neuron = start; neuron < end; ++neuron) {
            uint64_t elapsed = b->tick - b->sensory_update_tick[neuron];
            if (elapsed) {
                b->efficacy[neuron] = fmin(1.0, b->efficacy[neuron] +
                    b->sensory_rec[neuron] * (double)elapsed);
                b->sensory_update_tick[neuron] = b->tick;
            }
            b->probability[neuron] = b->sensory_rate[neuron] * x * 0.001;
        }
    }
}

/* Avança uma célula sem entrada em forma fechada, preservando leakage/adaptação. */
static void advance_passive_gap(Brain *b, uint32_t j, uint64_t gap) {
    if (!gap) return;
    uint64_t blocked = (uint64_t)b->refractory[j];
    if (blocked > gap) blocked = gap;
    uint64_t active = gap - blocked;
    double adapt0 = b->adapt[j];
    int pool = b->pool_of_neuron[NS + j];
    double adapt_decay = b->adapt_decay[j];
    double decay = b->decay[j];
    if (active) {
        double decay_power, adapt_power, adapt_after_refractory, kernel;
        if (gap <= MAX_CACHED_GAP) {
            decay_power = b->passive_decay_power[pool][active];
            adapt_power = b->passive_adapt_power[pool][active];
            adapt_after_refractory = adapt0 * b->passive_adapt_power[pool][blocked];
            kernel = b->passive_kernel[pool][active];
        } else {
            adapt_after_refractory = adapt0 * pow(adapt_decay, (double)blocked);
            decay_power = pow(decay, (double)active);
            adapt_power = pow(adapt_decay, (double)active);
            kernel = fabs(decay - adapt_decay) < 1e-12
                ? (double)active * pow(decay, (double)(active - 1u))
                : (decay_power - adapt_power) / (decay - adapt_decay);
        }
        b->v[j] = b->v[j] * decay_power - adapt_after_refractory * kernel;
    }
    b->adapt[j] = gap <= MAX_CACHED_GAP ? adapt0 * b->passive_adapt_power[pool][gap]
                                       : adapt0 * pow(adapt_decay, (double)gap);
    b->refractory[j] -= (int)blocked;
}

static void record_spike(Brain *b, uint32_t neuron, double now_ms) {
    b->next_fired_ids[b->next_fired_count++] = neuron;
    if (!b->plastic_mark[neuron]) {
        b->plastic_mark[neuron] = 1;
        b->plastic_pre[b->plastic_pre_count++] = neuron;
    }
    b->last_spike_t[neuron] = now_ms;
    b->active_spikes_frame++;
    b->stats.spikes_emitted++;
    int pool = pre_pool_for_index(b, neuron);
    if (pool >= 0) b->pool_counts[pool] += 1.0;
}

static void substep(Brain *b) {
    b->tick++;
    b->active_target_count = 0;
    b->next_fired_count = 0;
    b->active_spikes_frame = 0;
    const double now_ms = b->time_ms;
    uint64_t phase_start_ns = monotonic_ns();

    /* Sensores por unidade em referência; o padrão usa propostas geométricas com thinning. */
    for (int c = 0; c < PESTINHA_CHANNELS; ++c) {
        if (b->levels[c] <= 0.0) continue;
        uint32_t start = (uint32_t)b->starts[c];
        uint32_t end = start + (uint32_t)b->sizes[c];
        b->stats.sensory_updates += end - start;
        double p = b->probability[start];
#ifdef CADDIN_SENSORY_BERNOULLI_REFERENCE
        for (uint32_t neuron = start; neuron < end; ++neuron) {
            b->stats.sensory_proposals++;
            uint64_t elapsed = b->tick - b->sensory_update_tick[neuron];
            b->efficacy[neuron] = fmin(1.0, b->efficacy[neuron] + b->sensory_rec[neuron] * (double)elapsed);
            b->sensory_update_tick[neuron] = b->tick;
            if (urand(b) < p * b->efficacy[neuron]) {
                b->efficacy[neuron] = fmax(0.0, b->efficacy[neuron] - b->sensory_d[neuron]);
                b->last_spike_t[neuron] = now_ms;
                b->next_fired_ids[b->next_fired_count++] = neuron;
                if (!b->plastic_mark[neuron]) { b->plastic_mark[neuron] = 1; b->plastic_pre[b->plastic_pre_count++] = neuron; }
                b->active_spikes_frame++; b->stats.spikes_emitted++; b->stats.sensory_spikes_emitted++;
                int pool = pre_pool_for_index(b, neuron);
                if (pool >= 0) b->pool_counts[pool] += 1.0;
            }
        }
#else
        if (p >= 1.0) {
            for (uint32_t neuron = start; neuron < end; ++neuron) {
                b->stats.sensory_proposals++;
                uint64_t elapsed = b->tick - b->sensory_update_tick[neuron];
                b->efficacy[neuron] = fmin(1.0, b->efficacy[neuron] + b->sensory_rec[neuron] * (double)elapsed);
                b->sensory_update_tick[neuron] = b->tick;
                if (urand(b) < b->efficacy[neuron]) {
                    b->efficacy[neuron] = fmax(0.0, b->efficacy[neuron] - b->sensory_d[neuron]);
                    b->last_spike_t[neuron] = now_ms;
                    b->next_fired_ids[b->next_fired_count++] = neuron;
                    if (!b->plastic_mark[neuron]) { b->plastic_mark[neuron] = 1; b->plastic_pre[b->plastic_pre_count++] = neuron; }
                    b->active_spikes_frame++; b->stats.spikes_emitted++; b->stats.sensory_spikes_emitted++;
                    int pool = pre_pool_for_index(b, neuron);
                    if (pool >= 0) b->pool_counts[pool] += 1.0;
                }
            }
        } else if (p > 0.0) {
            const double log_failure = log1p(-p);
            uint32_t cursor = start;
            while (cursor < end) {
                double skip_real = log(urand(b)) / log_failure;
                if (skip_real > (double)(end - cursor)) break;
                uint32_t skip = (uint32_t)skip_real;
                uint64_t candidate = (uint64_t)cursor + skip;
                if (candidate >= end) break;
                uint32_t neuron = (uint32_t)candidate;
                b->stats.sensory_proposals++;
                uint64_t elapsed = b->tick - b->sensory_update_tick[neuron];
                b->efficacy[neuron] = fmin(1.0, b->efficacy[neuron] + b->sensory_rec[neuron] * (double)elapsed);
                b->sensory_update_tick[neuron] = b->tick;
                if (urand(b) < b->efficacy[neuron]) {
                b->efficacy[neuron] = fmax(0.0, b->efficacy[neuron] - b->sensory_d[neuron]);
                b->last_spike_t[neuron] = now_ms;
                b->next_fired_ids[b->next_fired_count++] = neuron;
                if (!b->plastic_mark[neuron]) {
                    b->plastic_mark[neuron] = 1;
                    b->plastic_pre[b->plastic_pre_count++] = neuron;
                }
                b->active_spikes_frame++;
                b->stats.spikes_emitted++;
                b->stats.sensory_spikes_emitted++;
                int pool = pre_pool_for_index(b, neuron);
                if (pool >= 0) b->pool_counts[pool] += 1.0;
                }
                cursor = neuron + 1u;
            }
        }
#endif
    }
    uint64_t propagation_start_ns = monotonic_ns();
    b->stats.sensory_ns += propagation_start_ns - phase_start_ns;

    /* Propaga somente os IDs que dispararam no subpasso anterior. */
    for (uint32_t k = 0; k < b->fired_count; ++k) {
        uint32_t pre = b->fired_ids[k];
        double gain = b->pre_gain[pre];
        for (uint32_t syn = b->row_ptr[pre]; syn < b->row_ptr[pre + 1u]; ++syn) {
            uint32_t post_idx = b->post[syn];
            uint32_t j = post_idx - NS;
            if (b->drive_stamp[j] != b->tick) {
                b->drive_stamp[j] = b->tick;
                b->drive[j] = 0.0;
                b->active_targets[b->active_target_count++] = j;
            }
            b->drive[j] += b->w[syn] * gain;
            b->stats.synapses_traversed++;
        }
    }
    uint64_t integration_start_ns = monotonic_ns();
    b->stats.propagation_ns += integration_start_ns - propagation_start_ns;

    /* Referência densa compilável para comparar com o kernel de eventos. */
#ifdef CADDIN_DENSE_REFERENCE
    for (uint32_t j = 0; j < NL; ++j) {
        b->stats.neuron_updates++;
        if (b->refractory[j] > 0) {
            b->refractory[j]--;
            b->adapt[j] *= b->adapt_decay[j];
        } else {
            double drive = b->drive_stamp[j] == b->tick ? b->drive[j] : 0.0;
            b->v[j] = b->v[j] * b->decay[j] + drive - b->adapt[j];
            if (b->v[j] >= 1.0) {
                b->v[j] = 0.0;
                b->adapt[j] += b->adapt_b[j];
                b->refractory[j] = 3;
                record_spike(b, NS + j, now_ms);
            }
            b->adapt[j] *= b->adapt_decay[j];
        }
        b->last_update_tick[j] = b->tick;
    }
#else
    /* Integra apenas alvos recebidos. Estados silenciosos avançam lazy ao despertar. */
    for (uint32_t k = 0; k < b->active_target_count; ++k) {
        uint32_t j = b->active_targets[k];
        uint32_t neuron = NS + j;
        uint64_t elapsed = b->tick - b->last_update_tick[j];
        uint64_t gap = elapsed ? elapsed - 1u : 0u;
        advance_passive_gap(b, j, gap);
        b->stats.neuron_updates++;

        if (b->refractory[j] > 0) {
            b->refractory[j]--;
            b->adapt[j] *= b->adapt_decay[j];
        } else {
            b->v[j] = b->v[j] * b->decay[j] + b->drive[j] - b->adapt[j];
            if (b->v[j] >= 1.0) {
                b->v[j] = 0.0;
                b->adapt[j] += b->adapt_b[j];
                b->refractory[j] = 3;
                record_spike(b, neuron, now_ms);
            }
            b->adapt[j] *= b->adapt_decay[j];
        }
        b->last_update_tick[j] = b->tick;
    }
#endif
    uint64_t integration_end_ns = monotonic_ns();
    b->stats.integration_ns += integration_end_ns - integration_start_ns;

    uint32_t *previous_ids = b->fired_ids;
    b->fired_ids = b->next_fired_ids;
    b->next_fired_ids = previous_ids;
    b->fired_count = b->next_fired_count;
    b->next_fired_count = 0;
    b->time_ms += 1.0;
}

void brain_step(Brain *b, double milliseconds) {
    if (!b) return;
    int n = (int)fmax(1.0, fmin(400.0, round(milliseconds)));
    uint64_t start_ns = monotonic_ns();
    b->plastic_pre_count = 0;
    memset(b->plastic_mark, 0, (size_t)PESTINHA_NEURONS * sizeof(*b->plastic_mark));
    memset(&b->stats, 0, sizeof(b->stats));
    b->stats.dense_neuron_updates = (uint64_t)n * PESTINHA_NEURONS;

    for (int i = 0; i < NPOOLS; ++i) b->pool_counts[i] = 0.0;
    for (int i = 0; i < n; ++i) substep(b);

    for (int i = 0; i < NPOOLS; ++i) {
        double r = b->pool_counts[i] / (n * 0.001 * b->sizes[i]);
        double alpha = 1.0 - exp(-(n * 0.001) / 0.5);
        b->pool_rates[i] += alpha * (r - b->pool_rates[i]);
        b->pool_counts[i] = 0.0;
    }
    uint64_t end_ns = monotonic_ns();
    b->stats.elapsed_ns = end_ns >= start_ns ? end_ns - start_ns : 0;
}

double brain_motor(const Brain *b, int i) {
    return (b && i >= 0 && i < PESTINHA_MOTORS) ?
        b->pool_rates[PESTINHA_CHANNELS + PESTINHA_INTERNEURONS + i] : 0.0;
}

double brain_interneuron(const Brain *b, int i) {
    return (b && i >= 0 && i < PESTINHA_INTERNEURONS) ?
        b->pool_rates[PESTINHA_CHANNELS + i] : 0.0;
}

double brain_efficacy(const Brain *b, int c) {
    if (!b || c < 0 || c >= PESTINHA_CHANNELS) return 0.0;
    double sum = 0.0;
    for (uint32_t j = 0; j < (uint32_t)b->sizes[c]; ++j) {
        uint32_t neuron = (uint32_t)b->starts[c] + j;
        uint64_t elapsed = b->tick - b->sensory_update_tick[neuron];
        sum += fmin(1.0, b->efficacy[neuron] + b->sensory_rec[neuron] * (double)elapsed);
    }
    return sum / b->sizes[c];
}

int brain_active_spike_count(const Brain *b) {
    return b ? b->active_spikes_frame : 0;
}

double brain_total_plastic_weight(const Brain *b) {
    return b ? b->total_plastic_delta : 0.0;
}

/* Aprendizado STDP (Spike-Timing-Dependent Plasticity) Trifatorial com Homeostase */
void brain_learn_stdp(Brain *b, double reward_factor, double dt_sec) {
    if (!b) return;
    double now = b->time_ms;
    double total_delta = 0.0;
    double decay_homeo = exp(-dt_sec / 10.0); /* Tempo de meia-vida homeostático */

    for (uint32_t i = 0; i < b->plastic_pre_count; ++i) {
        uint32_t pre = b->plastic_pre[i];
        double t_pre = b->last_spike_t[pre];
        if (now - t_pre > 100.0) continue; /* janela temporal limite */

        for (uint32_t syn = b->row_ptr[pre]; syn < b->row_ptr[pre + 1]; ++syn) {
            uint32_t post_idx = b->post[syn];
            float w_cur = b->w[syn];

            double t_post = b->last_spike_t[post_idx];
            double dt = t_post - t_pre; /* dt > 0 => Pre antes de Post (LTP) */
            double dw = 0.0;

            if (dt > 0.0 && dt < 50.0) {
                /* LTP (Potenciação) */
                int delta_ms = (int)dt;
                double factor = dt == (double)delta_ms ? b->stdp_ltp_factor[delta_ms]
                                                       : exp(-dt / STDP_TAU_POS);
                dw = STDP_A_POS * factor;
            } else if (dt < 0.0 && dt > -50.0) {
                /* LTD (Depressão) */
                int delta_ms = (int)(-dt);
                double factor = -dt == (double)delta_ms ? b->stdp_ltd_factor[delta_ms]
                                                        : exp(dt / STDP_TAU_NEG);
                dw = -STDP_A_NEG * factor;
            }

            /* Modulação Trifatorial por Recompensa/Dopamina */
            dw *= (1.0 + reward_factor);

            /* Atualiza o peso sináptico */
            float w_new = (float)(w_cur + dw);

            /* Homeostase Sináptica: relaxamento em direção ao peso base w0 quantizado */
            float w_base = (float)b->w0[syn] * (1.0f / 127.0f);
            w_new = (float)(w_new * decay_homeo + w_base * (1.0 - decay_homeo));

            /* Limites sinápticos */
            if (w_new < STDP_W_MIN) w_new = STDP_W_MIN;
            if (w_new > STDP_W_MAX) w_new = STDP_W_MAX;

            b->w[syn] = w_new;
            total_delta += fabs(w_new - w_base);
        }
    }
    b->total_plastic_delta = total_delta;
}

void brain_reset_habituation(Brain *b) {
    if (!b) return;
    for (uint32_t i = 0; i < NS; ++i) {
        b->efficacy[i] = 1.0;
        b->pre_gain[i] = 1.0;
        b->sensory_update_tick[i] = b->tick;
    }
}

double brain_pool_rate(const Brain *b, int i) {
    return (b && i >= 0 && i < NPOOLS) ? b->pool_rates[i] : 0.0;
}

int brain_pool_count(void) { return NPOOLS; }
uint32_t brain_synapse_count(const Brain *b) { return b ? b->n_synapses : 0; }

bool brain_step_stats(const Brain *b, BrainStepStats *out) {
    if (!b || !out) return false;
    *out = b->stats;
    return true;
}

#ifdef BRAIN_STEP_STATS_AVAILABLE
static uint64_t bench_hash_bytes(uint64_t hash, const void *data, size_t size) {
    const unsigned char *bytes = data;
    for (size_t i = 0; i < size; ++i) hash = (hash ^ bytes[i]) * 1099511628211ULL;
    return hash;
}

static uint64_t bench_hash_double(uint64_t hash, double value) {
    int64_t quantized = isfinite(value) ? (int64_t)llround(value * 1e9) : INT64_MIN;
    return bench_hash_bytes(hash, &quantized, sizeof(quantized));
}

static int compare_u32(const void *left, const void *right) {
    uint32_t a = *(const uint32_t *)left, b = *(const uint32_t *)right;
    return (a > b) - (a < b);
}

uint64_t brain_bench_state_hash(Brain *b) {
    if (!b) return 0;
    uint64_t hash = 1469598103934665603ULL;
    hash = bench_hash_bytes(hash, &b->rng, sizeof(b->rng));
    hash = bench_hash_bytes(hash, &b->tick, sizeof(b->tick));
    hash = bench_hash_double(hash, b->time_ms);
    for (uint32_t i = 0; i < NS; ++i) {
        uint64_t elapsed = b->tick - b->sensory_update_tick[i];
        b->efficacy[i] = fmin(1.0, b->efficacy[i] + b->sensory_rec[i] * (double)elapsed);
        b->sensory_update_tick[i] = b->tick;
        hash = bench_hash_double(hash, b->efficacy[i]);
        hash = bench_hash_double(hash, b->last_spike_t[i]);
    }
    for (uint32_t j = 0; j < NL; ++j) {
        uint64_t elapsed = b->tick - b->last_update_tick[j];
        if (elapsed) advance_passive_gap(b, j, elapsed);
        b->last_update_tick[j] = b->tick;
        hash = bench_hash_double(hash, b->v[j]);
        hash = bench_hash_double(hash, b->adapt[j]);
        hash = bench_hash_bytes(hash, &b->refractory[j], sizeof(b->refractory[j]));
        hash = bench_hash_double(hash, b->last_spike_t[NS + j]);
    }
    for (uint32_t i = 0; i < PESTINHA_NEURONS; ++i)
        hash = bench_hash_double(hash, b->pre_gain[i]);
    for (uint32_t i = 0; i < NS; ++i)
        hash = bench_hash_double(hash, b->probability[i]);
    for (uint32_t i = 0; i < b->n_synapses; ++i)
        hash = bench_hash_bytes(hash, &b->w[i], sizeof(b->w[i]));
    for (int i = 0; i < NPOOLS; ++i)
        hash = bench_hash_double(hash, b->pool_rates[i]);

    uint32_t *ids = malloc((size_t)b->fired_count * sizeof(*ids));
    if (!ids && b->fired_count) return 0;
    if (b->fired_count) {
        memcpy(ids, b->fired_ids, (size_t)b->fired_count * sizeof(*ids));
        qsort(ids, b->fired_count, sizeof(*ids), compare_u32);
    }
    hash = bench_hash_bytes(hash, &b->fired_count, sizeof(b->fired_count));
    hash = bench_hash_bytes(hash, ids, (size_t)b->fired_count * sizeof(*ids));
    free(ids);
    return hash;
}
#endif

bool brain_pool_info(const Brain *b, int pool_index, BrainPoolInfo *out) {
    if (!b || !out || pool_index < 0 || pool_index >= NPOOLS) return false;
    out->name = b->names[pool_index];
    out->start = (uint32_t)b->starts[pool_index];
    out->size = (uint32_t)b->sizes[pool_index];
    out->kind = (BrainPoolKind)b->kind[pool_index];
    return out->name != NULL && out->size > 0;
}

typedef struct {
    uint32_t magic;
    uint32_t format_version;
    uint32_t neuron_count;
    uint32_t synapse_count;
    uint32_t topology_hash;
} BrainSparseHeader;

#define BRAIN_SPARSE_MAGIC 0x314E5242u /* BRN1 */

static uint32_t brain_topology_hash(const uint32_t *row_ptr, const uint32_t *post) {
    uint32_t h = 2166136261u;
    for (uint32_t i = 0; i <= PESTINHA_NEURONS; ++i) {
        h = (h ^ row_ptr[i]) * 16777619u;
    }
    uint32_t n = row_ptr[PESTINHA_NEURONS];
    for (uint32_t i = 0; i < n; ++i) h = (h ^ post[i]) * 16777619u;
    return h;
}

void brain_save(const Brain *b, const char *path) {
    if (!b || !path || !b->row_ptr) return;
    BrainSparseHeader header = {
        BRAIN_SPARSE_MAGIC, 1u, PESTINHA_NEURONS, b->n_synapses,
        brain_topology_hash(b->row_ptr, b->post)
    };
    size_t rows_bytes = ((size_t)PESTINHA_NEURONS + 1u) * sizeof(uint32_t);
    size_t posts_bytes = (size_t)b->n_synapses * sizeof(uint32_t);
    size_t weights_bytes = (size_t)b->n_synapses * sizeof(float);
    size_t bases_bytes = (size_t)b->n_synapses * sizeof(int8_t);
    size_t payload_size = sizeof(header) + rows_bytes + posts_bytes + weights_bytes + bases_bytes;
    uint8_t *payload = malloc(payload_size);
    if (!payload) return;
    uint8_t *p = payload;
    memcpy(p, &header, sizeof(header)); p += sizeof(header);
    memcpy(p, b->row_ptr, rows_bytes); p += rows_bytes;
    memcpy(p, b->post, posts_bytes); p += posts_bytes;
    memcpy(p, b->w, weights_bytes); p += weights_bytes;
    memcpy(p, b->w0, bases_bytes);
    (void)persist_save_atomic(path, PESTINHA_NEURONS, payload, payload_size);
    free(payload);
}

static int brain_load_sparse(Brain *b, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    PersistHeader persist_header;
    if (fread(&persist_header, sizeof(persist_header), 1, f) != 1 ||
        persist_header.magic != PERSIST_MAGIC || persist_header.version != PERSIST_VERSION ||
        persist_header.neuron_count != PESTINHA_NEURONS) {
        fclose(f);
        return 0;
    }
    size_t max_payload = sizeof(BrainSparseHeader) +
        ((size_t)PESTINHA_NEURONS + 1u) * sizeof(uint32_t) +
        (size_t)PESTINHA_NEURONS * MAX_OUT_DEGREE * (sizeof(uint32_t) + sizeof(float) + sizeof(int8_t));
    if (persist_header.payload_size < sizeof(BrainSparseHeader) || persist_header.payload_size > max_payload) {
        fclose(f);
        return 0;
    }
    fclose(f);

    size_t bytes_read = 0;
    uint8_t *payload = malloc(persist_header.payload_size);
    if (!payload) return 0;
    if (!persist_load_safe(path, PESTINHA_NEURONS, payload, persist_header.payload_size, &bytes_read) ||
        bytes_read != persist_header.payload_size) {
        free(payload);
        return 0;
    }
    const uint8_t *p = payload;
    BrainSparseHeader header;
    memcpy(&header, p, sizeof(header)); p += sizeof(header);
    size_t rows_bytes = ((size_t)PESTINHA_NEURONS + 1u) * sizeof(uint32_t);
    size_t posts_bytes = (size_t)header.synapse_count * sizeof(uint32_t);
    size_t weights_bytes = (size_t)header.synapse_count * sizeof(float);
    size_t bases_bytes = (size_t)header.synapse_count * sizeof(int8_t);
    size_t expected = sizeof(header) + rows_bytes + posts_bytes + weights_bytes + bases_bytes;
    if (header.magic != BRAIN_SPARSE_MAGIC || header.format_version != 1u ||
        header.neuron_count != PESTINHA_NEURONS ||
        (uint64_t)header.synapse_count > (uint64_t)PESTINHA_NEURONS * MAX_OUT_DEGREE ||
        expected != bytes_read) {
        free(payload);
        return 0;
    }
    const uint32_t *saved_rows = (const uint32_t *)p; p += rows_bytes;
    const uint32_t *saved_posts = (const uint32_t *)p; p += posts_bytes;
    const float *saved_weights = (const float *)p; p += weights_bytes;
    const int8_t *saved_bases = (const int8_t *)p;

    int valid_rows = saved_rows[0] == 0 && saved_rows[PESTINHA_NEURONS] == header.synapse_count;
    for (uint32_t i = 0; valid_rows && i < PESTINHA_NEURONS; ++i) {
        if (saved_rows[i] > saved_rows[i + 1] || saved_rows[i + 1] > header.synapse_count) valid_rows = 0;
    }
    for (uint32_t i = 0; valid_rows && i < header.synapse_count; ++i) {
        if (saved_posts[i] < NS || saved_posts[i] >= PESTINHA_NEURONS ||
            !isfinite(saved_weights[i]) || saved_weights[i] < STDP_W_MIN ||
            saved_weights[i] > STDP_W_MAX) valid_rows = 0;
    }
    int valid_degree = 1;
    for (uint32_t i = 0; valid_rows && i < PESTINHA_NEURONS; ++i)
        if (saved_rows[i + 1u] - saved_rows[i] > MAX_OUT_DEGREE) valid_degree = 0;
    int valid_targets = 1;
    for (uint32_t pre = 0; valid_rows && valid_targets && pre < PESTINHA_NEURONS; ++pre) {
        for (uint32_t syn = saved_rows[pre]; syn < saved_rows[pre + 1u]; ++syn) {
            if (saved_posts[syn] < (uint32_t)b->starts[PESTINHA_CHANNELS] ||
                saved_posts[syn] >= PESTINHA_NEURONS) { valid_targets = 0; break; }
            for (uint32_t prior = saved_rows[pre]; prior < syn; ++prior)
                if (saved_posts[prior] == saved_posts[syn]) { valid_targets = 0; break; }
        }
    }
    if (!valid_rows || !valid_degree || !valid_targets || brain_topology_hash(saved_rows, saved_posts) != header.topology_hash) {
        free(payload);
        return 0;
    }
    uint32_t *new_rows = malloc(rows_bytes);
    uint32_t *new_posts = malloc(posts_bytes);
    float *new_weights = malloc(weights_bytes);
    int8_t *new_bases = malloc(bases_bytes);
    if (!new_rows || (header.synapse_count && (!new_posts || !new_weights || !new_bases))) {
        free(payload); free(new_rows); free(new_posts); free(new_weights); free(new_bases); return 0;
    }
    memcpy(new_rows, saved_rows, rows_bytes);
    memcpy(new_posts, saved_posts, posts_bytes);
    memcpy(new_weights, saved_weights, weights_bytes);
    memcpy(new_bases, saved_bases, bases_bytes);
    brain_release_connectome(b);
    b->row_ptr = new_rows; b->post = new_posts; b->w = new_weights; b->w0 = new_bases;
    b->n_synapses = header.synapse_count;
    free(payload);
    return 1;
}

static int brain_migrate_legacy_dense(Brain *b, const float *legacy) {
    /* Projeta a memória antiga para a réplica 0 correspondente, preservando a
       topologia nova sem reconstruir uma matriz N×NL. */
    uint32_t *old_pre_map = malloc((size_t)LEGACY_NEURONS * sizeof(*old_pre_map));
    uint32_t *old_post_map = malloc((size_t)LEGACY_NEURONS * sizeof(*old_post_map));
    if (!old_pre_map || !old_post_map) { free(old_pre_map); free(old_post_map); return 0; }
    /* O formato legado não gravava os parâmetros da rede; mantém apenas pesos
       correspondentes à topologia CSR criada nesta inicialização. */
    for (uint32_t old_pre = 0; old_pre < LEGACY_NEURONS; ++old_pre) {
        uint32_t start, local;
        if (old_pre < LEGACY_SENSORY) {
            uint32_t pool = old_pre / 8u;
            local = old_pre % 8u;
            start = pool * PESTINHA_SENSORY_PER_POOL;
        } else if (old_pre < LEGACY_SENSORY + LEGACY_ASSOCIATIVE) {
            uint32_t offset = old_pre - LEGACY_SENSORY;
            uint32_t pool = offset / 36u;
            local = offset % 36u;
            start = NS + pool * PESTINHA_INTER_PER_POOL;
        } else {
            uint32_t offset = old_pre - LEGACY_SENSORY - LEGACY_ASSOCIATIVE;
            uint32_t pool = offset / 16u;
            local = offset % 16u;
            start = NS + PESTINHA_INTERNEURONS * PESTINHA_INTER_PER_POOL + pool * PESTINHA_MOTOR_PER_POOL;
        }
        old_pre_map[old_pre] = start + local * PESTINHA_NEURON_SCALE;
    }
    for (uint32_t old_post = 0; old_post < LEGACY_NEURONS; ++old_post) {
        if (old_post < LEGACY_SENSORY) { old_post_map[old_post] = UINT32_MAX; continue; }
        uint32_t start, local;
        if (old_post < LEGACY_SENSORY + LEGACY_ASSOCIATIVE) {
            uint32_t offset = old_post - LEGACY_SENSORY;
            uint32_t pool = offset / 36u;
            local = offset % 36u;
            start = NS + pool * PESTINHA_INTER_PER_POOL;
        } else {
            uint32_t offset = old_post - LEGACY_SENSORY - LEGACY_ASSOCIATIVE;
            uint32_t pool = offset / 16u;
            local = offset % 16u;
            start = NS + PESTINHA_INTERNEURONS * PESTINHA_INTER_PER_POOL + pool * PESTINHA_MOTOR_PER_POOL;
        }
        old_post_map[old_post] = start + local * PESTINHA_NEURON_SCALE;
    }
    for (uint32_t old_pre = 0; old_pre < LEGACY_NEURONS; ++old_pre) {
        uint32_t new_pre = old_pre_map[old_pre];
        for (uint32_t old_post = LEGACY_SENSORY; old_post < LEGACY_NEURONS; ++old_post) {
            float weight = legacy[(size_t)old_pre * LEGACY_FREE + (old_post - LEGACY_SENSORY)];
            if (weight == 0.0f) continue;
            weight /= (float)PESTINHA_NEURON_SCALE;
            uint32_t new_post = old_post_map[old_post];
            uint32_t syn = b->row_ptr[new_pre];
            while (syn < b->row_ptr[new_pre + 1] && b->post[syn] != new_post) syn++;
            if (syn < b->row_ptr[new_pre + 1]) {
                b->w[syn] = weight;
                int q = (int)lrintf(weight * 127.0f);
                if (q > 127) q = 127;
                if (q < -128) q = -128;
                b->w0[syn] = (int8_t)q;
            }
        }
    }
    free(old_pre_map); free(old_post_map);
    return 1;
}

int brain_load(Brain *b, const char *path) {
    if (!b || !path) return 0;
    if (brain_load_sparse(b, path)) return 1;

    const size_t legacy_bytes = (size_t)LEGACY_NEURONS * LEGACY_FREE * sizeof(float);
    float *legacy = malloc(legacy_bytes);
    if (!legacy) return 0;
    size_t bytes_read = 0;
    int loaded = persist_load_safe(path, LEGACY_NEURONS, legacy, legacy_bytes, &bytes_read) &&
                 bytes_read == legacy_bytes;
    if (!loaded) {
        FILE *f = fopen(path, "rb");
        if (f) {
            uint32_t magic = 0, count = 0;
            loaded = fread(&magic, sizeof(magic), 1, f) == 1 && magic == 0x50455354u &&
                     fread(&count, sizeof(count), 1, f) == 1 && count == LEGACY_NEURONS &&
                     fread(legacy, 1, legacy_bytes, f) == legacy_bytes;
            fclose(f);
        }
    }
    if (loaded) loaded = brain_migrate_legacy_dense(b, legacy);
    free(legacy);
    if (loaded) brain_save(b, path);
    return loaded;
}
