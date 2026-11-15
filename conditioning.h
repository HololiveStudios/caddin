#ifndef PESTINHA_CONDITIONING_H
#define PESTINHA_CONDITIONING_H

#include "runtime_types.h"

void conditioning_init(ConditioningState *c);
void conditioning_reward(ConditioningState *c, double amount);
void conditioning_punish(ConditioningState *c, double amount);
void conditioning_update(ConditioningState *c, double dt);

#endif /* PESTINHA_CONDITIONING_H */
