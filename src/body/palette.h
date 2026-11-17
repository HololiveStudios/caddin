#ifndef PET_BODY_PALETTE_H
#define PET_BODY_PALETTE_H

#include "../core/fixed.h"
#include "../core/neuromod.h"
#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t pupil_dilation; /* 0 = contraída, 255 = dilatada ao máximo */
} EmotionalAura;

void emotional_aura_compute(const Neuromodulators *nm, fix16_t local_light, EmotionalAura *out_aura);

#endif /* PET_BODY_PALETTE_H */
