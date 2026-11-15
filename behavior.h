#ifndef PESTINHA_BEHAVIOR_H
#define PESTINHA_BEHAVIOR_H

#include "runtime_types.h"
#include "neuro.h"

void behavior_init(Behavior *b, Emotion *e, FaceState *f);
void behavior_update(Behavior *behavior, Drives *drives, ConditioningState *cond,
                     Brain *brain, double now_t, double dt);
void behavior_update_emotion(Emotion *emotion, Behavior *behavior, FaceState *face,
                            Brain *brain, double now_t, double dt);

#endif /* PESTINHA_BEHAVIOR_H */
