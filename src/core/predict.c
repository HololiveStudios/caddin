#include "predict.h"
#include <stdlib.h>

void predict_init(TemporalPredictor *pred) {
    if (!pred) return;
    pred->last_event_tick = 0;
    pred->expected_interval = 0;
    pred->tolerance = 15; /* Tolerância em ticks */
    pred->confidence = 0;
    pred->expecting_event = false;
    pred->mismatch_fired = false;
}

void predict_register_event(TemporalPredictor *pred, uint32_t current_tick, fix16_t *out_reward_da) {
    if (!pred) return;

    if (pred->last_event_tick == 0) {
        pred->last_event_tick = current_tick;
        return;
    }

    uint32_t delta = current_tick - pred->last_event_tick;
    pred->last_event_tick = current_tick;

    /* Se o intervalo for razoável (entre 20 e 1000 ticks) */
    if (delta >= 20 && delta <= 1000) {
        if (pred->expected_interval == 0) {
            pred->expected_interval = delta;
            pred->confidence = 64;
        } else {
            int32_t diff = (int32_t)delta - (int32_t)pred->expected_interval;
            if (abs(diff) <= (int32_t)pred->tolerance) {
                /* Confirmação preditiva: reforço dopaminérgico */
                if (pred->confidence < 240) pred->confidence += 32;
                if (out_reward_da) {
                    *out_reward_da = fix_from_float(0.4f);
                }
            } else {
                /* Ajuste suave do intervalo esperado */
                pred->expected_interval = (uint32_t)((pred->expected_interval * 3 + delta) / 4);
                if (pred->confidence > 32) pred->confidence -= 16;
            }
        }
        pred->expecting_event = true;
        pred->mismatch_fired = false;
    }
}

bool predict_step(TemporalPredictor *pred, uint32_t current_tick, fix16_t *out_surprise_ne) {
    if (!pred || !pred->expecting_event || pred->expected_interval == 0) return false;

    uint32_t elapsed = current_tick - pred->last_event_tick;
    uint32_t deadline = pred->expected_interval + pred->tolerance;

    if (elapsed > deadline && !pred->mismatch_fired) {
        /* Quebra de expectativa temporal: Mismatch Spike! */
        pred->mismatch_fired = true;
        pred->confidence = (uint16_t)(pred->confidence >> 1); /* Queda de confiança */

        if (out_surprise_ne) {
            *out_surprise_ne = fix_from_float(0.7f); /* Surpresa (Noradrenalina/ACh) */
        }
        return true;
    }
    return false;
}
