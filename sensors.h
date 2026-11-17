#ifndef PESTINHA_SENSORS_H
#define PESTINHA_SENSORS_H

#include "runtime_types.h"
#include <stdbool.h>

typedef struct Sensors Sensors;

Sensors *sensors_open(void);
void sensors_close(Sensors *s);

/* Alimenta dados de física da janela a cada frame */
void sensors_feed_physics(Sensors *s, double dt,
                          bool is_dragging, double vx, double vy, double speed,
                          bool clicked, bool bumped,
                          int cursor_x, int cursor_y, int pet_x, int pet_y);

/* Alimenta tecla pressionada */
void sensors_register_key(Sensors *s, char c, double now_t);

/* Alimenta dados internos de homeostase para canais interoceptivos */
void sensors_feed_drives(Sensors *s, const Drives *drives, double reward_trace, double punish_trace);

/* Calcula inputs[64] para o cérebro SNN com base em TUDO que foi alimentado */
void sensors_poll(Sensors *s, double now_t, SensorReading *reading, double inputs[PESTINHA_CHANNELS]);

#endif /* PESTINHA_SENSORS_H */
