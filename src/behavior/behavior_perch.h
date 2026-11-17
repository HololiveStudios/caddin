#ifndef PET_BEHAVIOR_PERCH_H
#define PET_BEHAVIOR_PERCH_H

#include <stdbool.h>

typedef enum {
    PERCH_STATE_FREE = 0,
    PERCH_STATE_APPROACHING, /* Aproximando-se do galho (topo da janela) */
    PERCH_STATE_PERCHED,     /* Pousado sobre a borda superior */
    PERCH_STATE_LEAVING      /* Saindo do poleiro */
} PerchState;

typedef struct {
    PerchState state;
    float perch_x;
    float perch_y;
    float hold_timer;
    bool  is_perched;
} BehaviorPerch;

void behavior_perch_init(BehaviorPerch *p);

/* Atualiza pouso na borda superior da janela.
 * Retorna true se estiver atualmente pousado ou alinhando sobre a janela. */
bool behavior_perch_update(BehaviorPerch *p, float dt,
                           float pet_x, float pet_y,
                           int win_x, int win_y, int win_w, int win_h,
                           float energy, float stress,
                           float *out_dx, float *out_dy);

#endif /* PET_BEHAVIOR_PERCH_H */
