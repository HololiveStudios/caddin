#ifndef PESTINHA_HOMEOSTASIS_H
#define PESTINHA_HOMEOSTASIS_H

#include "runtime_types.h"

void homeostasis_init(Drives *drives);
void homeostasis_update(Drives *drives, Behavior *behavior, double dt, int moving,
                        double arousal, const double inputs[PESTINHA_CHANNELS]);
void homeostasis_event(Drives *drives, const char *event_type);

#endif /* PESTINHA_HOMEOSTASIS_H */
