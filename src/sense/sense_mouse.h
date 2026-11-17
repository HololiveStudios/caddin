#ifndef PET_SENSE_MOUSE_H
#define PET_SENSE_MOUSE_H

#include "../core/fixed.h"
#include <stdbool.h>

typedef struct {
    int last_x;
    int last_y;
    int vx;
    int vy;
    int last_vx;
    int last_vy;
    int shake_reversals;
    int shake_window_ticks;
    fix16_t current_speed;
    fix16_t current_acceleration;
    fix16_t friction_force_x;
    fix16_t friction_force_y;
    bool is_contacting_body;
} SenseMouse;

void    sense_mouse_init(SenseMouse *sm);
void    sense_mouse_update(SenseMouse *sm, int mouse_x, int mouse_y, int pet_x, int pet_y, int pet_radius);
fix16_t sense_mouse_get_shake_intensity(const SenseMouse *sm);
bool    sense_mouse_is_rubbing(const SenseMouse *sm);

#endif /* PET_SENSE_MOUSE_H */
