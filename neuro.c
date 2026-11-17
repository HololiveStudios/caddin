#include "neuro.h"
#include "network_data.h"
#include "src/core/persist.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NS (PESTINHA_CHANNELS * 8)  /* 256 neurônios sensoriais */
#define NL (PESTINHA_NEURONS - NS)  /* 768 neurônios livres (inter + motores) */
#define NPOOLS (PESTINHA_CHANNELS + PESTINHA_INTERNEURONS + PESTINHA_MOTORS)

#define STDP_TAU_POS 20.0  /* ms */
#define STDP_TAU_NEG 20.0  /* ms */
#define STDP_A_POS   0.005
#define STDP_A_NEG   0.0045
#define STDP_W_MIN  -0.5f
#define STDP_W_MAX   1.0f

struct Brain {
    unsigned int rng;

    int starts[NPOOLS];
    int sizes[NPOOLS];
    int kind[NPOOLS];
    const char *names[NPOOLS];

    float w[PESTINHA_NEURONS * NL];     /* matriz de conexões sinápticas */
    int8_t w0[PESTINHA_NEURONS * NL];   /* pesos originais base quantizados para homeostase (768 KB em vez de 3.15 MB) */

    double v[NL];                       /* potencial de membrana (LIF) */
    double adapt[NL];
    double adapt_b[NL];
    double adapt_decay[NL];
    double decay[NL];
    int refractory[NL];

    uint8_t spike[PESTINHA_NEURONS];
    double last_spike_t[PESTINHA_NEURONS];
    double pre_gain[PESTINHA_NEURONS];

    double sensory_rate[NS];
    double sensory_d[NS];
    double sensory_rec[NS];
    double efficacy[NS];
    double probability[NS];

    double levels[PESTINHA_CHANNELS];
    double pool_counts[NPOOLS];
    double pool_rates[NPOOLS];

    int active_spikes_frame;
    double total_plastic_delta;
    double time_ms;
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

static int find_pool(const Brain *b, const char *n) {
    for (int i = 0; i < NPOOLS; ++i) {
        if (!strcmp(n, b->names[i])) return i;
    }
    return -1;
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

static void connect_pools(Brain *b, const char *pre, const char *post, double weight, double density) {
    int a = find_pool(b, pre), z = find_pool(b, post);
    if (a < 0 || z < 0) return;

    for (int i = 0; i < b->sizes[a]; ++i) {
        int pre_idx = b->starts[a] + i;
        for (int j = 0; j < b->sizes[z]; ++j) {
            int post_idx = b->starts[z] + j;
            if (post_idx >= NS && urand(b) < density) {
                int col = post_idx - NS;
                int pos = pre_idx * NL + col;
                b->w[pos] = (float)weight;
                int q = (int)round(weight * 127.0);
                if (q > 127) q = 127;
                if (q < -128) q = -128;
                b->w0[pos] = (int8_t)q;
            }
        }
    }
}

Brain *brain_create(unsigned int seed, const char *temperament) {
    Brain *b = calloc(1, sizeof(*b));
    int cursor = 0;
    (void)temperament;
    if (!b) return NULL;

    b->rng = seed ? seed : 42u;

    /* 1. Inicializa Canais Sensoriais (64 pools x 4 neurônios = 256) */
    for (int i = 0; i < PESTINHA_CHANNELS; ++i) {
        int n = SENS_POOLS[i].n;
        b->starts[i] = cursor;
        b->sizes[i]  = n;
        b->kind[i]   = 0;
        b->names[i]  = SENS_POOLS[i].name;
        cursor += n;
        for (int j = 0; j < n; ++j) {
            int q = b->starts[i] + j;
            b->sensory_rate[q] = SENS_POOLS[i].rate;
            b->sensory_d[q]    = SENS_POOLS[i].d;
            b->sensory_rec[q]  = 0.001 / SENS_POOLS[i].tau_rec;
            b->efficacy[q]     = 1.0;
        }
    }

    /* 2. Inicializa Interneurônios (32 pools x 18 neurônios = 576) */
    for (int i = 0; i < PESTINHA_INTERNEURONS; ++i) {
        int p = PESTINHA_CHANNELS + i;
        b->starts[p] = cursor;
        b->sizes[p]  = INTER_POOLS[i].n;
        b->kind[p]   = !strcmp(INTER_POOLS[i].kind, "inh") ? 2 : 1;
        b->names[p]  = INTER_POOLS[i].name;
        cursor += b->sizes[p];

        for (int j = 0; j < b->sizes[p]; ++j) {
            int q = b->starts[p] + j - NS;
            b->adapt_b[q]     = INTER_POOLS[i].b;
            b->adapt_decay[q] = exp(-0.001 / INTER_POOLS[i].bt);
            b->decay[q]       = exp(-1.0 / INTER_POOLS[i].tau);
        }
    }

    /* 3. Inicializa Motores (24 pools x 8 neurônios = 192) */
    for (int i = 0; i < PESTINHA_MOTORS; ++i) {
        int p = PESTINHA_CHANNELS + PESTINHA_INTERNEURONS + i;
        b->starts[p] = cursor;
        b->sizes[p]  = MOTOR_POOLS[i].n;
        b->kind[p]   = 3;
        b->names[p]  = MOTOR_POOLS[i].name;
        cursor += b->sizes[p];

        for (int j = 0; j < b->sizes[p]; ++j) {
            int q = b->starts[p] + j - NS;
            b->adapt_b[q]     = MOTOR_POOLS[i].b;
            b->adapt_decay[q] = exp(-0.001 / MOTOR_POOLS[i].bt);
            b->decay[q]       = exp(-1.0 / MOTOR_POOLS[i].tau);
        }
    }

    if (cursor != PESTINHA_NEURONS) {
        free(b);
        return NULL;
    }

    for (int i = 0; i < PESTINHA_NEURONS; ++i) {
        b->pre_gain[i] = 1.0;
        b->last_spike_t[i] = -1000.0;
    }

    /* 4. Estabelece Conexões por Definição */
    int num_conns = (int)(sizeof(DEFAULT_CONNS) / sizeof(DEFAULT_CONNS[0]));
    for (int i = 0; i < num_conns; ++i) {
        connect_pools(b, DEFAULT_CONNS[i].pre, DEFAULT_CONNS[i].post,
                      DEFAULT_CONNS[i].weight, DEFAULT_CONNS[i].density);
    }

    /* 5. Adiciona Conectividade Recorrente Aleatória entre Interneurônios */
    for (int i = 0; i < PESTINHA_INTERNEURONS; ++i) {
        int pre_p = PESTINHA_CHANNELS + i;
        for (int j = 0; j < PESTINHA_INTERNEURONS; ++j) {
            if (i == j) continue;
            int post_p = PESTINHA_CHANNELS + j;
            connect_pools(b, b->names[pre_p], b->names[post_p], 0.15, 0.12);
        }
    }

    return b;
}

void brain_destroy(Brain *b) {
    if (b) free(b);
}

void brain_stimulate(Brain *b, const double values[PESTINHA_CHANNELS]) {
    if (!b) return;
    memset(b->probability, 0, sizeof(b->probability));

    for (int c = 0; c < PESTINHA_CHANNELS; ++c) {
        double x = fmax(0.0, fmin(1.0, values[c]));
        b->levels[c] = x;
        for (int j = 0; j < b->sizes[c]; ++j) {
            int n = b->starts[c] + j;
            b->probability[n] = b->sensory_rate[n] * x * 0.001;
        }
    }
}

static void substep(Brain *b) {
    double drive[NL];
    memset(drive, 0, sizeof(drive));
    b->active_spikes_frame = 0;

    /* A. Atualiza neurônios sensoriais */
    for (int i = 0; i < NS; ++i) {
        if (b->probability[i] > 0.0 && urand(b) < b->probability[i]) {
            b->spike[i] = 1;
            b->last_spike_t[i] = b->time_ms;
            b->efficacy[i] = fmax(0.0, b->efficacy[i] - b->sensory_d[i]);
            b->active_spikes_frame++;
        } else {
            b->spike[i] = 0;
            b->efficacy[i] = fmin(1.0, b->efficacy[i] + b->sensory_rec[i]);
        }
    }

    /* B. Propaga estímulo pré-sináptico */
    for (int i = 0; i < PESTINHA_NEURONS; ++i) {
        if (b->spike[i]) {
            double gain = b->pre_gain[i];
            int offset = i * NL;
            for (int j = 0; j < NL; ++j) {
                float weight = b->w[offset + j];
                if (weight != 0.0f) {
                    drive[j] += weight * gain;
                }
            }
        }
    }

    /* C. Atualiza potencial de membrana (LIF) dos neurônios livres */
    for (int j = 0; j < NL; ++j) {
        int n_idx = NS + j;

        if (b->refractory[j] > 0) {
            b->refractory[j]--;
            b->spike[n_idx] = 0;
        } else {
            b->v[j] = b->v[j] * b->decay[j] + drive[j] - b->adapt[j];
            if (b->v[j] >= 1.0) {
                b->spike[n_idx] = 1;
                b->last_spike_t[n_idx] = b->time_ms;
                b->v[j] = 0.0;
                b->adapt[j] += b->adapt_b[j];
                b->refractory[j] = 3;
                b->active_spikes_frame++;
            } else {
                b->spike[n_idx] = 0;
            }
        }
        b->adapt[j] *= b->adapt_decay[j];
    }

    /* D. Contabiliza disparos nos pools */
    for (int i = 0; i < NPOOLS; ++i) {
        int start = b->starts[i];
        int sz = b->sizes[i];
        for (int j = 0; j < sz; ++j) {
            if (b->spike[start + j]) {
                b->pool_counts[i] += 1.0;
            }
        }
    }

    b->time_ms += 1.0;
}

void brain_step(Brain *b, double milliseconds) {
    if (!b) return;
    int n = (int)fmax(1.0, fmin(400.0, round(milliseconds)));

    for (int i = 0; i < n; ++i) {
        substep(b);
    }

    for (int i = 0; i < NPOOLS; ++i) {
        double r = b->pool_counts[i] / (n * 0.001 * b->sizes[i]);
        double alpha = 1.0 - exp(-(n * 0.001) / 0.5);
        b->pool_rates[i] += alpha * (r - b->pool_rates[i]);
        b->pool_counts[i] = 0.0;
    }
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
    for (int j = 0; j < b->sizes[c]; ++j) {
        sum += b->efficacy[b->starts[c] + j];
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

    for (int i = 0; i < PESTINHA_NEURONS; ++i) {
        double t_pre = b->last_spike_t[i];
        if (now - t_pre > 100.0) continue; /* janela temporal limite */

        int offset = i * NL;
        for (int j = 0; j < NL; ++j) {
            int post_idx = NS + j;
            float w_cur = b->w[offset + j];
            if (w_cur == 0.0f) continue;

            double t_post = b->last_spike_t[post_idx];
            double dt = t_post - t_pre; /* dt > 0 => Pre antes de Post (LTP) */
            double dw = 0.0;

            if (dt > 0.0 && dt < 50.0) {
                /* LTP (Potenciação) */
                dw = STDP_A_POS * exp(-dt / STDP_TAU_POS);
            } else if (dt < 0.0 && dt > -50.0) {
                /* LTD (Depressão) */
                dw = -STDP_A_NEG * exp(dt / STDP_TAU_NEG);
            }

            /* Modulação Trifatorial por Recompensa/Dopamina */
            dw *= (1.0 + reward_factor);

            /* Atualiza o peso sináptico */
            float w_new = (float)(w_cur + dw);

            /* Homeostase Sináptica: relaxamento em direção ao peso base w0 quantizado */
            float w_base = (float)b->w0[offset + j] * (1.0f / 127.0f);
            w_new = (float)(w_new * decay_homeo + w_base * (1.0 - decay_homeo));

            /* Limites sinápticos */
            if (w_new < STDP_W_MIN) w_new = STDP_W_MIN;
            if (w_new > STDP_W_MAX) w_new = STDP_W_MAX;

            b->w[offset + j] = w_new;
            total_delta += fabs(w_new - w_base);
        }
    }
    b->total_plastic_delta = total_delta;
}

void brain_reset_habituation(Brain *b) {
    if (!b) return;
    for (int i = 0; i < NS; ++i) {
        b->efficacy[i] = 1.0;
        b->pre_gain[i] = 1.0;
    }
}

void brain_save(const Brain *b, const char *path) {
    if (!b || !path) return;
    /* Persistência v2 com cabeçalho atômico e integridade CRC32 */
    size_t payload_size = sizeof(float) * PESTINHA_NEURONS * NL;
    persist_save_atomic(path, PESTINHA_NEURONS, b->w, payload_size);
}

int brain_load(Brain *b, const char *path) {
    if (!b || !path) return 0;
    size_t payload_size = sizeof(float) * PESTINHA_NEURONS * NL;
    size_t bytes_read = 0;

    /* 1. Tenta carregar no formato moderno Persistência v2 */
    if (persist_load_safe(path, PESTINHA_NEURONS, b->w, payload_size, &bytes_read)) {
        if (bytes_read == payload_size) return 1;
    }

    /* 2. Fallback: formato legado v1 ('PEST') para não perder cérebros antigos */
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    uint32_t magic = 0, count = 0;
    if (fread(&magic, sizeof(magic), 1, f) != 1 || magic != 0x50455354) {
        fclose(f); return 0;
    }
    if (fread(&count, sizeof(count), 1, f) != 1 || count != PESTINHA_NEURONS) {
        fclose(f); return 0;
    }
    if (fread(b->w, sizeof(float), PESTINHA_NEURONS * NL, f) != payload_size) {
        fclose(f); return 0;
    }
    fclose(f);

    /* Converte automaticamente para o formato v2 na próxima gravação */
    brain_save(b, path);
    return 1;
}
