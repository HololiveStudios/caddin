#ifndef PET_BEHAVIOR_HIDE_HUNT_H
#define PET_BEHAVIOR_HIDE_HUNT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    HIDE_HUNT_IDLE = 0,
    HIDE_HUNT_HIDING,   /* Escondendo-se 80% atrás de borda sob medo */
    HIDE_HUNT_PEEKING,  /* Olhando timidamente para fora */
    HIDE_HUNT_STALKING, /* Aproximação furtiva do cursor (caça/brincadeira) */
    HIDE_HUNT_POUNCING  /* Bote lúdico no cursor */
} HideHuntState;

typedef struct {
    HideHuntState state;
    float state_timer;
    float target_x;
    float target_y;
    float stalk_speed;
    bool  is_hidden;
    uint32_t last_eval_tick;
} BehaviorHideHunt;

void behavior_hide_hunt_init(BehaviorHideHunt *hh);

/* Atualiza a máquina de esconder/caçar com base em estresse, brincadeira, posição do mouse e janela ativa.
 * Retorna offset de deslocamento sugerido (dx, dy). */
void behavior_hide_hunt_update(BehaviorHideHunt *hh, float dt,
                               float pet_x, float pet_y,
                               float mouse_x, float mouse_y,
                               int win_x, int win_y, int win_w, int win_h,
                               float stress, float energy, float curiosity,
                               float *out_dx, float *out_dy);

#endif /* PET_BEHAVIOR_HIDE_HUNT_H */
