#ifndef PET_NEUROMOD_H
#define PET_NEUROMOD_H

#include "fixed.h"
#include <stdbool.h>

typedef struct {
    fix16_t da;    /* Dopamina (0.0 a 2.0, base ~0.5) */
    fix16_t ne;    /* Noradrenalina (0.0 a 2.0, base ~0.2) */
    fix16_t ser;   /* Serotonina (0.0 a 2.0, base ~0.6) */
    fix16_t ach;   /* Acetilcolina (0.0 a 2.0, base ~0.4) */
    fix16_t cort;  /* Cortisol (0.0 a 2.0, base ~0.1) */

    /* Níveis basais */
    fix16_t base_da;
    fix16_t base_ne;
    fix16_t base_ser;
    fix16_t base_ach;
    fix16_t base_cort;

    /* Taxas de decaimento em shift (valores maiores = decaimento mais lento) */
    int decay_shift;
} Neuromodulators;

void    neuromod_init(Neuromodulators *nm);
void    neuromod_step(Neuromodulators *nm);
void    neuromod_inject_da(Neuromodulators *nm, fix16_t delta);
void    neuromod_inject_ne(Neuromodulators *nm, fix16_t delta);
void    neuromod_inject_cort(Neuromodulators *nm, fix16_t delta);
void    neuromod_inject_ser(Neuromodulators *nm, fix16_t delta);
void    neuromod_inject_ach(Neuromodulators *nm, fix16_t delta);

/* Fator de ganho sensorial: NE alta aumenta sensibilidade */
fix16_t neuromod_get_sensory_gain(const Neuromodulators *nm);

/* Arco reflexo nociceptivo: verifica impacto brusco/sacudida súbita */
bool    neuromod_check_nociceptive_reflex(Neuromodulators *nm, fix16_t violent_shake_intensity, fix16_t *out_motor_jolt);

#endif /* PET_NEUROMOD_H */
