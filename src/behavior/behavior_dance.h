#ifndef PET_BEHAVIOR_DANCE_H
#define PET_BEHAVIOR_DANCE_H

#include <stdbool.h>
#include <stdint.h>
#include "../animation/softbody.h"

typedef enum {
    DANCE_STYLE_IDLE = 0,
    DANCE_STYLE_BOUNCE,   /* Salto elástico na batida */
    DANCE_STYLE_SWAY,     /* Balanço lateral pendular no compasso */
    DANCE_STYLE_WIGGLE,   /* Vibração rápida e jiggle de alta energia */
    DANCE_STYLE_GROOVE    /* Combinação soltinha de squash-stretch e balanço */
} DanceStyle;

typedef struct {
    bool       is_dancing;
    DanceStyle style;
    float      dance_timer;
    float      style_timer;
    int        beat_counter;
    float      sway_vel_x;
    float      bounce_y;
    bool       should_chirp;
} BehaviorDance;

void behavior_dance_init(BehaviorDance *d);

/* Atualiza a dança sincronizada com o ritmo musical.
 * Retorna true se estiver ativamente dançando.
 * Preenche out_dx (deslocamento lateral) e out_bob_y (elevação rítmica da face/corpo). */
bool behavior_dance_update(BehaviorDance *d, float dt,
                           bool is_music, float bpm, float beat_phase,
                           bool is_on_beat, float beat_intensity,
                           SoftBody *sb, float *out_dx, float *out_bob_y);

#endif /* PET_BEHAVIOR_DANCE_H */
