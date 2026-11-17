#ifndef PET_RNG_H
#define PET_RNG_H

#include <stdint.h>

typedef struct {
    uint32_t s[4];
} RngState;

void     rng_seed(RngState *rng, uint64_t seed);
uint32_t rng_u32(RngState *rng);
float    rng_f32(RngState *rng); /* [0.0, 1.0) */
int32_t  rng_range(RngState *rng, int32_t min, int32_t max);

#endif /* PET_RNG_H */
