#include "sensors.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct Sensors {
    double last_input_t;
    double key_press_interval;
    int burst_counter;
    double session_start_t;
};

Sensors *sensors_open(void) {
    Sensors *s = calloc(1, sizeof(*s));
    if (s) {
        s->last_input_t = 0.0;
        s->session_start_t = 0.0;
    }
    return s;
}

void sensors_close(Sensors *s) {
    if (s) free(s);
}

void sensors_register_key(Sensors *s, char c, double now_t) {
    if (!s) return;
    double dt = now_t - s->last_input_t;
    s->key_press_interval = dt;
    s->last_input_t = now_t;

    if (dt < 0.20) {
        s->burst_counter++;
    } else {
        s->burst_counter = 1;
    }
    (void)c;
}

void sensors_poll(Sensors *s, double now_t, SensorReading *reading, double inputs[PESTINHA_CHANNELS]) {
    if (!inputs) return;
    memset(inputs, 0, sizeof(double) * PESTINHA_CHANNELS);

    double idle = now_t - (s ? s->last_input_t : 0.0);
    if (reading) {
        reading->idle_time = idle;
    }

    /* Ativa os canais sensoriais correspondentes */
    if (idle < 0.5) {
        inputs[0] = 0.85; /* TYPING */
        if (s && s->key_press_interval < 0.15) {
            inputs[1] = 0.90; /* KEY_FAST */
        } else {
            inputs[2] = 0.60; /* KEY_SLOW */
        }
    } else if (idle > 5.0) {
        inputs[3] = 0.75; /* IDLE */
        inputs[36] = 0.80; /* IDLE_LONG */
    }

    if (s && s->burst_counter > 5) {
        inputs[46] = 0.95; /* BURST_INPUT */
    }

    /* Ciclo Circadiano do sistema (Hora do dia) */
    time_t rawtime;
    time(&rawtime);
    struct tm *timeinfo = localtime(&rawtime);
    if (timeinfo) {
        int hour = timeinfo->tm_hour;
        if (hour >= 6 && hour < 18) {
            inputs[32] = 0.70; /* TIME_DAY */
            inputs[48] = 0.80; /* CIRCADIAN_PEAK */
        } else {
            inputs[33] = 0.85; /* TIME_NIGHT */
            inputs[49] = 0.80; /* CIRCADIAN_LOW */
        }
    }

    /* Pulso biológico do loop */
    inputs[39] = (sin(now_t * 3.14159) + 1.0) * 0.5; /* TICK_PULSE */
}
