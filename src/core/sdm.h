#ifndef PET_CORE_SDM_H
#define PET_CORE_SDM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Hipervetores de 2048 bits (32 palavras uint64_t = 256 bytes) */
#define SDM_SCHEMA_VERSION 2u
#define SDM_DIM_WORDS 32
#define SDM_DIM_BITS  (SDM_DIM_WORDS * 64)
#define SDM_LOCATIONS 128
#define SDM_K_ACTIVE  8
#define SDM_EPISODE_CAPACITY 256
#define SDM_EPISODE_SCHEMA_VERSION 1u

typedef struct {
    uint64_t words[SDM_DIM_WORDS];
} Hypervector;

typedef struct {
    Hypervector value;
    uint8_t strength;
    uint8_t valid;
} SdmAssociation;

typedef struct {
    Hypervector key;
    Hypervector value;
    uint32_t writes;
    uint8_t valid;
} SdmEpisode;

typedef struct {
    SdmEpisode episodes[SDM_EPISODE_CAPACITY];
    uint32_t next_slot;
    uint32_t count;
} SdmEpisodeIndex;

typedef struct {
    Hypervector value;
    float similarity;
    uint32_t writes;
    uint8_t valid;
} SdmEpisodeRecall;

typedef struct {
    Hypervector address[SDM_LOCATIONS];
    int8_t      counters[SDM_LOCATIONS][SDM_DIM_BITS];
    uint32_t    total_writes;
} SparseDistributedMemory;

/* Inicialização da SDM com posições pseudo-aleatórias determinísticas */
void sdm_init(SparseDistributedMemory *sdm, uint32_t seed);

/* Operações Fundamentais de Computação Hiperdimensional */
Hypervector hv_zero(void);
Hypervector hv_random(uint32_t *rng_state);
Hypervector hv_bind(Hypervector a, Hypervector b);      /* XOR */
Hypervector hv_bundle(const Hypervector *vecs, size_t n); /* Votação por maioria */
uint32_t    hv_hamming_dist(Hypervector a, Hypervector b);
float       hv_similarity(Hypervector a, Hypervector b);  /* 1.0 = idêntico, 0.0 = ortogonal, -1.0 = inverso */

/* Gravação e Recordação Associativa de Episódios */
void sdm_store(SparseDistributedMemory *sdm, Hypervector address, Hypervector data);
bool sdm_recall(const SparseDistributedMemory *sdm, Hypervector address, Hypervector *out_data);
void sdm_association_store(SparseDistributedMemory *sdm, Hypervector address,
                           Hypervector value);
bool sdm_association_recall(const SparseDistributedMemory *sdm, Hypervector address,
                            SdmAssociation *out_association);
void sdm_episode_index_init(SdmEpisodeIndex *index);
/* Returns false at capacity when all episodes are consolidated; no strong record is evicted. */
bool sdm_episode_index_store(SdmEpisodeIndex *index, Hypervector key, Hypervector value);
bool sdm_episode_index_recall(const SdmEpisodeIndex *index, Hypervector key,
                              SdmEpisodeRecall *out_recall);
void sdm_episode_index_consolidate(SdmEpisodeIndex *index);
size_t sdm_episode_index_serialize(const SdmEpisodeIndex *index, uint8_t *out, size_t cap);
bool sdm_episode_index_deserialize(SdmEpisodeIndex *index, const uint8_t *in, size_t len);

/* Poda no sono: decai contadores em direção a zero, preservando padrões consolidados */
void sdm_consolidate_sleep(SparseDistributedMemory *sdm);

/* Métricas para experimentos reproduzíveis de associação e esquecimento. */
uint32_t sdm_active_counter_count(const SparseDistributedMemory *sdm);

/* Serialização e Desserialização para Persistência */
size_t sdm_serialize(const SparseDistributedMemory *sdm, uint8_t *out, size_t cap);
bool   sdm_deserialize(SparseDistributedMemory *sdm, const uint8_t *in, size_t len);
bool sdm_deserialize_compatible(SparseDistributedMemory *sdm, const uint8_t *in, size_t len,
                                bool *legacy_format);

/* Codificação e Decodificação de Episódios do Caddin */
typedef struct {
    char   window_name[64];
    int    hour;             /* 0..23 */
    int    valence_level;    /* -2 (muito triste) a +2 (muito feliz) */
    int    stress_level;     /* 0 (calmo) a 3 (muito estressado) */
    int    event_type;       /* 0=rotina, 1=carinho, 2=susto, 3=erro_codigo, 4=sucesso_codigo */
} EpisodeDescriptor;

Hypervector sdm_encode_episode(const EpisodeDescriptor *ep);

#endif /* PET_CORE_SDM_H */
