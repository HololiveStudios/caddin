#ifndef PET_SENSE_FATIGUE_H
#define PET_SENSE_FATIGUE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float  fatigue_level;        /* 0.0 (descansado) a 1.0 (exaustão) */
    double continuous_work_s;    /* Tempo contínuo ativo sem pausa */
    double last_activity_t;      /* Última interação de teclado/mouse */
    int    backspace_count;      /* Erros de digitação acumulados */
    int    total_keys_count;
    double mouse_jitter_accum;
    int    last_cursor_x;
    int    last_cursor_y;
} SenseFatigue;

void  sense_fatigue_init(SenseFatigue *sf);
void  sense_fatigue_register_key(SenseFatigue *sf, char key, double now_s);
void  sense_fatigue_register_mouse(SenseFatigue *sf, int cursor_x, int cursor_y, double now_s);
void  sense_fatigue_update(SenseFatigue *sf, double dt, double now_s);
float sense_fatigue_get_level(const SenseFatigue *sf);
bool  sense_fatigue_is_exhausted(const SenseFatigue *sf); /* Nível > 0.75 */

#endif /* PET_SENSE_FATIGUE_H */
