#ifndef CORTEX_AUTONOMY_H
#define CORTEX_AUTONOMY_H

#include "window.h"
#include "softbody.h"
#include "emotions.h"
#include <stdbool.h>

typedef struct Brain Brain;

typedef struct {
    double decision_timer;     /* Tempo até a próxima ação autônoma */
    double observe_timer;      /* Tempo observando o cursor */
    int    last_mouse_x;
    int    last_mouse_y;
    double mouse_activity_t;   /* Atividade do usuário no mouse */
    float  smooth_look_x;
    float  smooth_look_y;
} AutonomyState;

void autonomy_init   (AutonomyState *au);
void autonomy_update (AutonomyState *au, double dt, PetWindow *win,
                      SoftBody *sb, EmotionMachine *em, const Brain *brain);

#endif /* CORTEX_AUTONOMY_H */
