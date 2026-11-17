#include "behavior_startle_relief.h"

void behavior_startle_relief_init(BehaviorStartleRelief *sr) {
    if (!sr) return;
    sr->phase = STARTLE_PHASE_CALM;
    sr->timer = 0.0f;
    sr->shock_intensity = 0.0f;
    sr->relief_triggered = false;
}

void behavior_startle_relief_trigger(BehaviorStartleRelief *sr, float intensity) {
    if (!sr) return;
    sr->phase = STARTLE_PHASE_SHOCK;
    sr->timer = 0.0f;
    sr->shock_intensity = (intensity > 1.0f) ? 1.0f : (intensity < 0.1f ? 0.1f : intensity);
    sr->relief_triggered = false;
}

bool behavior_startle_relief_update(BehaviorStartleRelief *sr, float dt,
                                    float *out_squish_mod, float *out_serotonin_boost) {
    if (!sr) return false;
    if (out_squish_mod) *out_squish_mod = 0.0f;
    if (out_serotonin_boost) *out_serotonin_boost = 0.0f;

    if (sr->phase == STARTLE_PHASE_CALM) {
        return false;
    }

    sr->timer += dt;

    if (sr->phase == STARTLE_PHASE_SHOCK) {
        /* Contrações espasmódicas na Fase 1 */
        if (out_squish_mod) {
            *out_squish_mod = 0.40f * sr->shock_intensity;
        }

        /* Duração do choque de 0.8 a 1.4s antes da transição para a Fase 2 (Alívio) */
        if (sr->timer >= 1.2f) {
            sr->phase = STARTLE_PHASE_RELIEF;
            sr->timer = 0.0f;
            sr->relief_triggered = true;
            return true; /* Notifica início do alívio */
        }
    }
    else if (sr->phase == STARTLE_PHASE_RELIEF) {
        /* Fase de Alívio: relaxamento muscular suave e injeção de serotonina calmante */
        float relief_progress = sr->timer / 1.5f; /* 1.5s de alívio */
        if (relief_progress > 1.0f) relief_progress = 1.0f;

        if (out_squish_mod) {
            /* Expansão/alongamento compensatório */
            *out_squish_mod = -0.15f * (1.0f - relief_progress);
        }
        if (out_serotonin_boost) {
            *out_serotonin_boost = 0.35f * (1.0f - relief_progress);
        }

        if (sr->timer >= 1.5f) {
            sr->phase = STARTLE_PHASE_CALM;
            sr->timer = 0.0f;
            sr->relief_triggered = false;
        }
    }

    return false;
}
