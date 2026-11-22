#include "sdm.h"
#include <string.h>
#include <math.h>
#include <stddef.h>
#include <limits.h>

#define SDM_V1_MAGIC 0x53444D31u /* SDM1: legado, somente contadores */
#define SDM_V2_MAGIC 0x53444D32u /* SDM2: schema e estado completo */
#define SDM_V2_HEADER_SIZE 24u
#define SDM_V2_PAYLOAD_SIZE (sizeof(((SparseDistributedMemory *)0)->address) + \
                             sizeof(((SparseDistributedMemory *)0)->counters) + sizeof(uint32_t))

static uint32_t sdm_crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)-(int32_t)(crc & 1u));
    }
    return crc ^ 0xFFFFFFFFu;
}

static void write_u32(uint8_t *out, uint32_t value) {
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
    out[2] = (uint8_t)(value >> 16);
    out[3] = (uint8_t)(value >> 24);
}

static uint32_t read_u32(const uint8_t *in) {
    return (uint32_t)in[0] | ((uint32_t)in[1] << 8) |
           ((uint32_t)in[2] << 16) | ((uint32_t)in[3] << 24);
}

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
    if (sdm->total_writes == 0) {
        *out_data = hv_zero();
        return false;
    }
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

void sdm_association_store(SparseDistributedMemory *sdm, Hypervector address,
                           Hypervector value) {
    if (!sdm) return;
    int locations[SDM_K_ACTIVE];
    find_k_nearest(sdm, address, locations);
    for (int k = 0; k < SDM_K_ACTIVE; ++k) {
        int8_t *counter = sdm->counters[locations[k]];
        for (int bit = 0; bit < SDM_DIM_BITS; ++bit) {
            bool set = (value.words[bit / 64] & (1ULL << (bit % 64))) != 0;
            if (set) {
                if (counter[bit] < 127) counter[bit]++;
            } else if (counter[bit] > -128) {
                counter[bit]--;
            }
        }
    }
    if (sdm->total_writes != UINT32_MAX) sdm->total_writes++;
}

bool sdm_association_recall(const SparseDistributedMemory *sdm, Hypervector address,
                            SdmAssociation *out_association) {
    if (!sdm || !out_association) return false;
    memset(out_association, 0, sizeof(*out_association));
    bool any_active = false;
    for (int loc = 0; loc < SDM_LOCATIONS && !any_active; ++loc)
        for (int bit = 0; bit < SDM_DIM_BITS; ++bit)
            if (sdm->counters[loc][bit] != 0) { any_active = true; break; }
    if (!any_active) return true;

    Hypervector value;
    if (!sdm_recall(sdm, address, &value)) {
        out_association->valid = 1;
        out_association->strength = 1;
        out_association->value = hv_zero();
        return true;
    }
    out_association->value = value;
    out_association->strength = (uint8_t)(sdm->total_writes > UINT8_MAX ? UINT8_MAX : sdm->total_writes);
    out_association->valid = 1;
    return true;
}

void sdm_episode_index_init(SdmEpisodeIndex *index) {
    if (index) memset(index, 0, sizeof(*index));
}

bool sdm_episode_index_store(SdmEpisodeIndex *index, Hypervector key, Hypervector value) {
    if (!index) return false;
    uint32_t selected = UINT32_MAX;
    for (uint32_t i = 0; i < SDM_EPISODE_CAPACITY; ++i) {
        if (index->episodes[i].valid && hv_hamming_dist(index->episodes[i].key, key) == 0) {
            selected = i;
            break;
        }
    }
    if (selected == UINT32_MAX)
        for (uint32_t i = 0; i < SDM_EPISODE_CAPACITY; ++i)
            if (!index->episodes[i].valid) { selected = i; break; }
    if (selected == UINT32_MAX) {
        selected = index->next_slot;
        uint32_t lowest_writes = UINT32_MAX;
        for (uint32_t offset = 0; offset < SDM_EPISODE_CAPACITY; ++offset) {
            uint32_t candidate = (index->next_slot + offset) % SDM_EPISODE_CAPACITY;
            if (index->episodes[candidate].writes < lowest_writes) {
                lowest_writes = index->episodes[candidate].writes;
                selected = candidate;
            }
        }
        index->next_slot = (selected + 1u) % SDM_EPISODE_CAPACITY;
    }
    if (selected == UINT32_MAX) return false;
    SdmEpisode *episode = &index->episodes[selected];
    bool is_new = !episode->valid;
    if (is_new || hv_hamming_dist(episode->key, key) != 0) {
        episode->key = key;
        episode->value = value;
        episode->writes = 1;
        if (!episode->valid) episode->valid = 1;
        if (is_new && index->count < SDM_EPISODE_CAPACITY) index->count++;
    } else {
        /* Result vectors are replaced as a complete labeled outcome, avoiding bitwise mixture. */
        episode->value = value;
        if (episode->writes != UINT32_MAX) episode->writes++;
    }
    index->next_slot = (selected + 1u) % SDM_EPISODE_CAPACITY;
    return true;
}

bool sdm_episode_index_recall(const SdmEpisodeIndex *index, Hypervector key,
                              SdmEpisodeRecall *out_recall) {
    if (!index || !out_recall) return false;
    memset(out_recall, 0, sizeof(*out_recall));
    uint32_t best_slot = UINT32_MAX;
    uint32_t best_distance = UINT32_MAX;
    for (uint32_t i = 0; i < SDM_EPISODE_CAPACITY; ++i) {
        if (!index->episodes[i].valid) continue;
        uint32_t distance = hv_hamming_dist(index->episodes[i].key, key);
        if (distance == 0) {
            best_slot = i;
            best_distance = 0;
            break;
        }
        if (distance < best_distance) {
            best_distance = distance;
            best_slot = i;
        }
    }
    if (best_slot == UINT32_MAX) return true;
    const SdmEpisode *episode = &index->episodes[best_slot];
    out_recall->similarity = 1.0f - 2.0f * (float)best_distance / (float)SDM_DIM_BITS;
    out_recall->value = episode->value;
    out_recall->writes = episode->writes;
    out_recall->valid = 1;
    return true;
}

void sdm_episode_index_consolidate(SdmEpisodeIndex *index) {
    if (!index) return;
    for (uint32_t i = 0; i < SDM_EPISODE_CAPACITY; ++i) {
        SdmEpisode *episode = &index->episodes[i];
        if (!episode->valid) continue;
        if (episode->writes <= 1u) {
            memset(episode, 0, sizeof(*episode));
            if (index->count) index->count--;
        } else {
            if (episode->writes > 1u) episode->writes--;
        }
    }
    index->next_slot = 0;
    for (uint32_t i = 0; i < SDM_EPISODE_CAPACITY; ++i)
        if (!index->episodes[i].valid) { index->next_slot = i; break; }
}

size_t sdm_episode_index_serialize(const SdmEpisodeIndex *index, uint8_t *out, size_t cap) {
    if (!index || !out) return 0;
    const size_t payload_size = sizeof(index->next_slot) + sizeof(index->count) +
                                sizeof(index->episodes);
    const size_t total_size = 20u + payload_size;
    if (cap < total_size) return 0;
    write_u32(out, 0x45504931u); /* EPI1 */
    write_u32(out + 4, SDM_EPISODE_SCHEMA_VERSION);
    write_u32(out + 8, SDM_EPISODE_CAPACITY);
    write_u32(out + 12, (uint32_t)(payload_size + sizeof(uint32_t)));
    size_t offset = 16;
    memcpy(out + offset, &index->next_slot, sizeof(index->next_slot));
    offset += sizeof(index->next_slot);
    memcpy(out + offset, &index->count, sizeof(index->count));
    offset += sizeof(index->count);
    memcpy(out + offset, index->episodes, sizeof(index->episodes));
    write_u32(out + total_size - 4, sdm_crc32(out, total_size - 4));
    return total_size;
}

bool sdm_episode_index_deserialize(SdmEpisodeIndex *index, const uint8_t *in, size_t len) {
    if (!index || !in || len < 16u) return false;
    const size_t payload_size = sizeof(index->next_slot) + sizeof(index->count) +
                                sizeof(index->episodes);
    const size_t total_size = 20u + payload_size;
    if (len != total_size || read_u32(in) != 0x45504931u ||
        read_u32(in + 4) != SDM_EPISODE_SCHEMA_VERSION ||
        read_u32(in + 8) != SDM_EPISODE_CAPACITY ||
        read_u32(in + 12) != payload_size + sizeof(uint32_t) ||
        read_u32(in + total_size - 4) != sdm_crc32(in, total_size - 4)) return false;
    SdmEpisodeIndex decoded;
    size_t offset = 16;
    memcpy(&decoded.next_slot, in + offset, sizeof(decoded.next_slot));
    offset += sizeof(decoded.next_slot);
    memcpy(&decoded.count, in + offset, sizeof(decoded.count));
    offset += sizeof(decoded.count);
    memcpy(decoded.episodes, in + offset, sizeof(decoded.episodes));
    if (decoded.next_slot >= SDM_EPISODE_CAPACITY || decoded.count > SDM_EPISODE_CAPACITY)
        return false;
    uint32_t actual_count = 0;
    for (uint32_t i = 0; i < SDM_EPISODE_CAPACITY; ++i) {
        const SdmEpisode *episode = &decoded.episodes[i];
        if (episode->valid > 1) return false;
        if (episode->valid) {
            if (episode->writes == 0) return false;
            actual_count++;
        }
    }
    if (actual_count != decoded.count) return false;
    *index = decoded;
    return true;
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

uint32_t sdm_active_counter_count(const SparseDistributedMemory *sdm) {
    if (!sdm) return 0;
    uint32_t active = 0;
    for (int loc = 0; loc < SDM_LOCATIONS; ++loc) {
        for (int bit = 0; bit < SDM_DIM_BITS; ++bit) {
            active += sdm->counters[loc][bit] != 0;
        }
    }
    return active;
}

size_t sdm_serialize(const SparseDistributedMemory *sdm, uint8_t *out, size_t cap) {
    if (!sdm || !out) return 0;
    size_t needed = SDM_V2_HEADER_SIZE + SDM_V2_PAYLOAD_SIZE;
    if (cap < needed) return 0;

    write_u32(out, SDM_V2_MAGIC);
    write_u32(out + 4, SDM_SCHEMA_VERSION);
    write_u32(out + 8, SDM_LOCATIONS);
    write_u32(out + 12, SDM_DIM_BITS);
    write_u32(out + 16, SDM_K_ACTIVE);
    write_u32(out + 20, (uint32_t)SDM_V2_PAYLOAD_SIZE);
    size_t offset = SDM_V2_HEADER_SIZE;
    memcpy(out + offset, sdm->address, sizeof(sdm->address));
    offset += sizeof(sdm->address);
    memcpy(out + offset, sdm->counters, sizeof(sdm->counters));
    offset += sizeof(sdm->counters);
    memcpy(out + offset, &sdm->total_writes, sizeof(sdm->total_writes));
    uint32_t checksum = sdm_crc32(out, needed - sizeof(checksum));
    write_u32(out + needed - sizeof(checksum), checksum);
    return needed;
}

bool sdm_deserialize(SparseDistributedMemory *sdm, const uint8_t *in, size_t len) {
    return sdm_deserialize_compatible(sdm, in, len, NULL);
}

bool sdm_deserialize_compatible(SparseDistributedMemory *sdm, const uint8_t *in, size_t len,
                                bool *legacy_format) {
    if (legacy_format) *legacy_format = false;
    if (!sdm || !in || len < sizeof(uint32_t)) return false;
    SparseDistributedMemory decoded = *sdm;
    uint32_t magic = read_u32(in);

    if (magic == SDM_V1_MAGIC) {
        const size_t legacy_size = sizeof(uint32_t) + sizeof(sdm->counters);
        if (len != legacy_size) return false;
        for (size_t i = sizeof(uint32_t); i < len; ++i)
            if ((int8_t)in[i] == INT8_MIN) return false;
        memcpy(decoded.counters, in + sizeof(uint32_t), sizeof(decoded.counters));
        decoded.total_writes = 0;
        *sdm = decoded;
        if (legacy_format) *legacy_format = true;
        return true;
    }

    if (magic != SDM_V2_MAGIC || len != SDM_V2_HEADER_SIZE + SDM_V2_PAYLOAD_SIZE ||
        read_u32(in + 4) != SDM_SCHEMA_VERSION || read_u32(in + 8) != SDM_LOCATIONS ||
        read_u32(in + 12) != SDM_DIM_BITS || read_u32(in + 16) != SDM_K_ACTIVE ||
        read_u32(in + 20) != SDM_V2_PAYLOAD_SIZE) return false;

    uint32_t stored_crc = read_u32(in + len - sizeof(uint32_t));
    if (sdm_crc32(in, len - sizeof(uint32_t)) != stored_crc) return false;
    size_t offset = SDM_V2_HEADER_SIZE;
    memcpy(decoded.address, in + offset, sizeof(decoded.address));
    offset += sizeof(decoded.address);
    memcpy(decoded.counters, in + offset, sizeof(decoded.counters));
    offset += sizeof(decoded.counters);
    memcpy(&decoded.total_writes, in + offset, sizeof(decoded.total_writes));
    *sdm = decoded;
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
