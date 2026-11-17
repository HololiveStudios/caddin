#ifndef CORTEX_EMOTIONS_H
#define CORTEX_EMOTIONS_H

#include <stdbool.h>

typedef struct Brain Brain;

typedef enum {
    EMO_IDLE = 0,
    EMO_AFFECTION,   /* Carinho / afeto (toque suave contínuo, Enter/C) */
    EMO_CURIOUS,     /* Curiosidade (clique, toque leve, olhar ponteiro) */
    EMO_SURPRISED,   /* Susto / surpresa (arrasto rápido, salto, batida) */
    EMO_DIZZY,       /* Tontura (sacudir por > 1.4s) */
    EMO_ANNOYED      /* Irritado (sacudir excessivamente) */
} PetEmotion;

typedef struct {
    PetEmotion current;
    PetEmotion previous;

    double state_timer;      /* Tempo no estado atual */
    double cooldown_timer;   /* Tempo restante para voltar ao IDLE */

    /* Detector de agitação (shake detector) */
    double last_drag_vx;
    int    direction_changes;
    double shake_window_t;
    double shake_duration;

    /* Rastreador de carinho / arrasto suave sem flickering */
    double gentle_drag_t;

    /* Respiração idle */
    double breath_phase;
    float  breath_scale;

    /* Piscar */
    double blink_timer;
    double blink_phase;
    int    blinking;
    float  blink_lid;

    /* Olhar */
    float look_x, look_y;
    double look_timer;
} EmotionMachine;

void  emot_init   (EmotionMachine *em);
void  emot_update (EmotionMachine *em, double dt,
                   bool is_dragging, double drag_speed, double drag_vx,
                   bool was_clicked, bool was_bumped);
void  emot_sync_brain(EmotionMachine *em, const Brain *brain);
void  emot_trigger(EmotionMachine *em, PetEmotion new_emo, double duration);

#endif /* CORTEX_EMOTIONS_H */
