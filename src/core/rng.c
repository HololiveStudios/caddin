#include "rng.h"

static inline uint32_t rotl(const uint32_t x, int k) {
    return (x << k) | (x >> (32 - k));
}

/* SplitMix64 para inicializar os 4 estados de 32 bits a partir de uma seed de 64 bits */
static uint64_t splitmix64(uint64_t *x) {
    uint64_t z = (*x += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

void rng_seed(RngState *rng, uint64_t seed) {
    if (!rng) return;
    if (seed == 0) seed = 0x123456789abcdef0ULL;
    uint64_t sm_state = seed;
    uint64_t s0 = splitmix64(&sm_state);
    uint64_t s1 = splitmix64(&sm_state);
    rng->s[0] = (uint32_t)s0;
    rng->s[1] = (uint32_t)(s0 >> 32);
    rng->s[2] = (uint32_t)s1;
    rng->s[3] = (uint32_t)(s1 >> 32);
    if ((rng->s[0] | rng->s[1] | rng->s[2] | rng->s[3]) == 0) {
        rng->s[0] = 1;
    }
}

/* xoshiro128** */
uint32_t rng_u32(RngState *rng) {
    const uint32_t result = rotl(rng->s[1] * 5, 7) * 9;
    const uint32_t t = rng->s[1] << 9;

    rng->s[2] ^= rng->s[0];
    rng->s[3] ^= rng->s[1];
    rng->s[1] ^= rng->s[2];
    rng->s[0] ^= rng->s[3];

    rng->s[2] ^= t;
    rng->s[3] = rotl(rng->s[3], 11);

    return result;
}

float rng_f32(RngState *rng) {
    /* Retorna float em [0.0, 1.0) usando 24 bits da mantissa */
    return (float)(rng_u32(rng) >> 8) * (1.0f / 16777216.0f);
}

int32_t rng_range(RngState *rng, int32_t min, int32_t max) {
    if (min >= max) return min;
    uint32_t span = (uint32_t)(max - min + 1);
    return min + (int32_t)(rng_u32(rng) % span);
}
