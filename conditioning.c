#include "conditioning.h"
#include <math.h>

void conditioning_init(ConditioningState *c) {
    if (!c) return;
    c->reward_trace = 0.0;
    c->punishment_trace = 0.0;
    c->caress_count = 0.0;
    c->poke_count = 0.0;
}

void conditioning_reward(ConditioningState *c, double amount) {
    if (!c) return;
    c->reward_trace = fmin(1.0, c->reward_trace + amount);
}

void conditioning_punish(ConditioningState *c, double amount) {
    if (!c) return;
    c->punishment_trace = fmin(1.0, c->punishment_trace + amount);
}

void conditioning_update(ConditioningState *c, double dt) {
    if (!c) return;
    c->reward_trace = fmax(0.0, c->reward_trace - 0.05 * dt);
    c->punishment_trace = fmax(0.0, c->punishment_trace - 0.05 * dt);
}
