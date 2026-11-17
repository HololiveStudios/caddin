#include "neuromod.h"

#define NOCICEPTIVE_THRESHOLD fix_from_float(1.5f)

void neuromod_init(Neuromodulators *nm) {
    if (!nm) return;
    nm->base_da = fix_from_float(0.5f);
    nm->base_ne = fix_from_float(0.2f);
    nm->base_ser = fix_from_float(0.6f);
    nm->base_ach = fix_from_float(0.4f);
    nm->base_cort = fix_from_float(0.1f);

    nm->da = nm->base_da;
    nm->ne = nm->base_ne;
    nm->ser = nm->base_ser;
    nm->ach = nm->base_ach;
    nm->cort = nm->base_cort;

    nm->decay_shift = 7; /* Relaxamento gradual em direção aos níveis basais */
}

static inline fix16_t decay_towards(fix16_t current, fix16_t target, int shift) {
    fix16_t diff = target - current;
    return (fix16_t)(current + (diff >> shift));
}

void neuromod_step(Neuromodulators *nm) {
    if (!nm) return;
    nm->da = decay_towards(nm->da, nm->base_da, nm->decay_shift);
    nm->ne = decay_towards(nm->ne, nm->base_ne, nm->decay_shift);
    nm->ser = decay_towards(nm->ser, nm->base_ser, nm->decay_shift);
    nm->ach = decay_towards(nm->ach, nm->base_ach, nm->decay_shift);
    nm->cort = decay_towards(nm->cort, nm->base_cort, nm->decay_shift);
}

void neuromod_inject_da(Neuromodulators *nm, fix16_t delta) {
    if (nm) nm->da = sat_add16(nm->da, delta);
}

void neuromod_inject_ne(Neuromodulators *nm, fix16_t delta) {
    if (nm) nm->ne = sat_add16(nm->ne, delta);
}

void neuromod_inject_cort(Neuromodulators *nm, fix16_t delta) {
    if (nm) nm->cort = sat_add16(nm->cort, delta);
}

void neuromod_inject_ser(Neuromodulators *nm, fix16_t delta) {
    if (nm) nm->ser = sat_add16(nm->ser, delta);
}

void neuromod_inject_ach(Neuromodulators *nm, fix16_t delta) {
    if (nm) nm->ach = sat_add16(nm->ach, delta);
}

fix16_t neuromod_get_sensory_gain(const Neuromodulators *nm) {
    if (!nm) return FIX_ONE;
    /* Ganho = 1.0 + (NE * 0.5) */
    return sat_add16(FIX_ONE, nm->ne >> 1);
}

bool neuromod_check_nociceptive_reflex(Neuromodulators *nm, fix16_t violent_shake_intensity, fix16_t *out_motor_jolt) {
    if (!nm) return false;

    if (violent_shake_intensity > NOCICEPTIVE_THRESHOLD) {
        /* Disparo de alarme fisiológico instantâneo */
        neuromod_inject_ne(nm, fix_from_float(0.8f));
        neuromod_inject_cort(nm, fix_from_float(0.6f));

        if (out_motor_jolt) {
            *out_motor_jolt = fix_from_float(2.0f); /* Impulso motor reflexo espasmódico */
        }
        return true;
    }
    return false;
}
