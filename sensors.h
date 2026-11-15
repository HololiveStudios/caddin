#ifndef PESTINHA_SENSORS_H
#define PESTINHA_SENSORS_H

#include "runtime_types.h"

typedef struct Sensors Sensors;

Sensors *sensors_open(void);
void sensors_close(Sensors *s);
void sensors_poll(Sensors *s, double now_t, SensorReading *reading, double inputs[PESTINHA_CHANNELS]);
void sensors_register_key(Sensors *s, char c, double now_t);

#endif /* PESTINHA_SENSORS_H */
