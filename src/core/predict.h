#ifndef PET_PREDICT_H
#define PET_PREDICT_H

#include "fixed.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t last_event_tick;
    uint32_t expected_interval;
    uint32_t tolerance;
    uint16_t confidence;     /* Q0.8 [0..255] */
    bool     expecting_event;
    bool     mismatch_fired;
} TemporalPredictor;

void    predict_init(TemporalPredictor *pred);
void    predict_register_event(TemporalPredictor *pred, uint32_t current_tick, fix16_t *out_reward_da);
bool    predict_step(TemporalPredictor *pred, uint32_t current_tick, fix16_t *out_surprise_ne);

#endif /* PET_PREDICT_H */
