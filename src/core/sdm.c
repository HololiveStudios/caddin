#include "sdm.h"
#include <string.h>
#include <math.h>

/* PRNG ultrarrápido para preenchimento de hipervetores */
static inline uint32_t xorshift32(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static inline uint32_t popcount64(uint64_t x) {
#if defined(__GNUC__) || defined(__clang__)
    return (uint32_t)__builtin_popcountll(x);
#else
    /* Fallback padrão de popcount de Hamming */
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return (uint32_t)((x * 0x0101010101010101ULL) >> 56);
#endif
}

Hypervector hv_zero(void) {
    Hypervector hv;
    memset(&hv, 0, sizeof(hv));
    return hv;
}

Hypervector hv_random(uint32_t *rng_state) {
    Hypervector hv;
    for (int i = 0; i < SDM_DIM_WORDS; ++i) {
        uint64_t low  = (uint64_t)xorshift32(rng_state);
        uint64_t high = (uint64_t)xorshift32(rng_state);
        hv.words[i] = low | (high << 32);
    }
    return hv;
}

Hypervector hv_bind(Hypervector a, Hypervector b) {
    Hypervector res;
    for (int i = 0; i < SDM_DIM_WORDS; ++i) {
        res.words[i] = a.words[i] ^ b.words[i];
    }
    return res;
}

Hypervector hv_bundle(const Hypervector *vecs, size_t n) {
    Hypervector res = hv_zero();
    if (!vecs || n == 0) return res;

    int threshold = (int)(n / 2);
    for (int b = 0; b < SDM_DIM_BITS; ++b) {
        int word_idx = b / 64;
        int bit_idx  = b % 64;
        uint64_t mask = (1ULL << bit_idx);

        int count_ones = 0;
        for (size_t i = 0; i < n; ++i) {
            if (vecs[i].words[word_idx] & mask) {
                count_ones++;
            }
        }
        if (count_ones > threshold) {
            res.words[word_idx] |= mask;
        }
    }
    return res;
}

uint32_t hv_hamming_dist(Hypervector a, Hypervector b) {
    uint32_t dist = 0;
    for (int i = 0; i < SDM_DIM_WORDS; ++i) {
        dist += popcount64(a.words[i] ^ b.words[i]);
    }
    return dist;
}

float hv_similarity(Hypervector a, Hypervector b) {
    uint32_t dist = hv_hamming_dist(a, b);
    /* Distância esperada aleatória é 1024 (similaridade 0.0). Idêntico = dist 0 (simil 1.0). Inverso = 2048 (simil -1.0). */
    return 1.0f - (2.0f * (float)dist / (float)SDM_DIM_BITS);
}

void sdm_init(SparseDistributedMemory *sdm, uint32_t seed) {
    if (!sdm) return;
    memset(sdm, 0, sizeof(*sdm));
    uint32_t s = seed ? seed : 0x1337C0DE;

    for (int i = 0; i < SDM_LOCATIONS; ++i) {
        sdm->address[i] = hv_random(&s);
    }
}

/* Encontra os K locais com menor distância de Hamming */
static void find_k_nearest(const SparseDistributedMemory *sdm, Hypervector query, int out_indices[SDM_K_ACTIVE]) {
    uint32_t dists[SDM_LOCATIONS];
    for (int i = 0; i < SDM_LOCATIONS; ++i) {
        dists[i] = hv_hamming_dist(query, sdm->address[i]);
    }

    for (int k = 0; k < SDM_K_ACTIVE; ++k) {
        uint32_t min_d = UINT32_MAX;
        int best_idx = 0;
        for (int i = 0; i < SDM_LOCATIONS; ++i) {
            if (dists[i] < min_d) {
                min_d = dists[i];
                best_idx = i;
            }
        }
        out_indices[k] = best_idx;
        dists[best_idx] = UINT32_MAX; /* Marca como já selecionado */
    }
}

void sdm_store(SparseDistributedMemory *sdm, Hypervector address, Hypervector data) {
    if (!sdm) return;
    int k_indices[SDM_K_ACTIVE];
    find_k_nearest(sdm, address, k_indices);

    for (int k = 0; k < SDM_K_ACTIVE; ++k) {
        int loc = k_indices[k];
        for (int b = 0; b < SDM_DIM_BITS; ++b) {
            int word_idx = b / 64;
            int bit_idx  = b % 64;
            bool bit_val = (data.words[word_idx] & (1ULL << bit_idx)) != 0;

            int8_t *cnt = &sdm->counters[loc][b];
            if (bit_val) {
                if (*cnt < 127) (*cnt)++;
            } else {
                if (*cnt > -128) (*cnt)--;
            }
        }
    }
    sdm->total_writes++;
}

bool sdm_recall(const SparseDistributedMemory *sdm, Hypervector address, Hypervector *out_data) {
    if (!sdm || !out_data) return false;
    int k_indices[SDM_K_ACTIVE];
    find_k_nearest(sdm, address, k_indices);

    *out_data = hv_zero();
    int non_zero_sums = 0;

    for (int b = 0; b < SDM_DIM_BITS; ++b) {
        int sum = 0;
        for (int k = 0; k < SDM_K_ACTIVE; ++k) {
            sum += sdm->counters[k_indices[k]][b];
        }

        if (sum > 0) {
            int word_idx = b / 64;
            int bit_idx  = b % 64;
            out_data->words[word_idx] |= (1ULL << bit_idx);
            non_zero_sums++;
        } else if (sum < 0) {
            non_zero_sums++;
        }
    }

    return (non_zero_sums > (SDM_DIM_BITS / 8));
}

void sdm_consolidate_sleep(SparseDistributedMemory *sdm) {
    if (!sdm) return;
    for (int loc = 0; loc < SDM_LOCATIONS; ++loc) {
        for (int b = 0; b < SDM_DIM_BITS; ++b) {
            int8_t *cnt = &sdm->counters[loc][b];
            if (*cnt > 1) {
                *cnt -= (int8_t)(*cnt >> 2);
            } else if (*cnt < -1) {
                *cnt += (int8_t)((-(*cnt)) >> 2);
            } else {
                *cnt = 0; /* Poda ruídos efêmeros sem consolidação */
            }
        }
    }
}

size_t sdm_serialize(const SparseDistributedMemory *sdm, uint8_t *out, size_t cap) {
    if (!sdm || !out) return 0;
    size_t needed = sizeof(uint32_t) + sizeof(sdm->counters);
    if (cap < needed) return 0;

    uint32_t magic = 0x53444D31; /* "SDM1" */
    memcpy(out, &magic, 4);
    memcpy(out + 4, sdm->counters, sizeof(sdm->counters));
    return needed;
}

bool sdm_deserialize(SparseDistributedMemory *sdm, const uint8_t *in, size_t len) {
    if (!sdm || !in) return false;
    size_t needed = sizeof(uint32_t) + sizeof(sdm->counters);
    if (len < needed) return false;

    uint32_t magic = 0;
    memcpy(&magic, in, 4);
    if (magic != 0x53444D31) return false;

    memcpy(sdm->counters, in + 4, sizeof(sdm->counters));
    return true;
}

/* Hash determinístico de string para semente */
static uint32_t hash_str(const char *str) {
    uint32_t h = 2166136261u;
    if (!str) return h;
    while (*str) {
        h ^= (uint8_t)*str++;
        h *= 16777619u;
    }
    return h;
}

Hypervector sdm_encode_episode(const EpisodeDescriptor *ep) {
    if (!ep) return hv_zero();

    /* 1. Vetor de Janela */
    uint32_t seed_win = hash_str(ep->window_name);
    Hypervector hv_win = hv_random(&seed_win);

    /* 2. Vetor de Horário (0..23) */
    uint32_t seed_hour = 0xABCD0000 | (uint32_t)(ep->hour % 24);
    Hypervector hv_hour = hv_random(&seed_hour);

    /* 3. Vetor de Emoção (Valência + Estresse) */
    uint32_t seed_emo = 0x55AA0000 | (uint32_t)((ep->valence_level + 2) & 0x7) | ((uint32_t)(ep->stress_level & 0x7) << 4);
    Hypervector hv_emo = hv_random(&seed_emo);

    /* 4. Vetor de Evento */
    uint32_t seed_ev = 0xCAFE0000 | (uint32_t)(ep->event_type & 0xF);
    Hypervector hv_ev = hv_random(&seed_ev);

    /* Liga pares: (Janela * Horário) e (Emoção * Evento) */
    Hypervector context = hv_bind(hv_win, hv_hour);
    Hypervector outcome = hv_bind(hv_emo, hv_ev);

    /* Superposição (Bundle) para gerar a memória holística do episódio */
    Hypervector pair[2] = { context, outcome };
    return hv_bundle(pair, 2);
}
