#ifndef PET_BEHAVIOR_STARTLE_RELIEF_H
#define PET_BEHAVIOR_STARTLE_RELIEF_H

#include <stdbool.h>

typedef enum {
    STARTLE_PHASE_CALM = 0,
    STARTLE_PHASE_SHOCK,   /* Fase 1: Contrações rápidas e pânico imediato */
    STARTLE_PHASE_RELIEF   /* Fase 2: Relaxamento pós-susto, suspiro e alívio */
} StartlePhase;

typedef struct {
    StartlePhase phase;
    float timer;
    float shock_intensity;
    bool  relief_triggered;
} BehaviorStartleRelief;

void behavior_startle_relief_init(BehaviorStartleRelief *sr);

/* Dispara o susto (Fase 1) */
void behavior_startle_relief_trigger(BehaviorStartleRelief *sr, float intensity);

/* Atualiza a dinâmica bifásica.
 * Retorna true no exato momento em que o alívio (Fase 2) se inicia, para disparar som/expressão. */
bool behavior_startle_relief_update(BehaviorStartleRelief *sr, float dt,
                                    float *out_squish_mod, float *out_serotonin_boost);

#endif /* PET_BEHAVIOR_STARTLE_RELIEF_H */
