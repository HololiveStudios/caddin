#include "sense_mouse.h"
#include <stdlib.h>
#include <math.h>

void sense_mouse_init(SenseMouse *sm) {
    if (!sm) return;
    sm->last_x = 0;
    sm->last_y = 0;
    sm->vx = 0;
    sm->vy = 0;
    sm->last_vx = 0;
    sm->last_vy = 0;
    sm->shake_reversals = 0;
    sm->shake_window_ticks = 0;
    sm->current_speed = 0;
    sm->current_acceleration = 0;
    sm->friction_force_x = 0;
    sm->friction_force_y = 0;
    sm->is_contacting_body = false;
}

void sense_mouse_update(SenseMouse *sm, int mouse_x, int mouse_y, int pet_x, int pet_y, int pet_radius) {
    if (!sm) return;

    sm->vx = mouse_x - sm->last_x;
    sm->vy = mouse_y - sm->last_y;
    sm->last_x = mouse_x;
    sm->last_y = mouse_y;

    /* Aceleração = variação de velocidade */
    int ax = sm->vx - sm->last_vx;
    int ay = sm->vy - sm->last_vy;

    /* Detecção de inversão de direção rápida (sacudida) */
    if ((sm->vx > 10 && sm->last_vx < -10) || (sm->vx < -10 && sm->last_vx > 10) ||
        (sm->vy > 10 && sm->last_vy < -10) || (sm->vy < -10 && sm->last_vy > 10)) {
        sm->shake_reversals++;
    }

    sm->last_vx = sm->vx;
    sm->last_vy = sm->vy;

    sm->shake_window_ticks++;
    if (sm->shake_window_ticks >= 20) { /* Janela de ~40 ms */
        sm->shake_reversals = (sm->shake_reversals > 1) ? (sm->shake_reversals - 1) : 0;
        sm->shake_window_ticks = 0;
    }

    /* Velocidade e aceleração aproximadas em ponto fixo */
    float spd = sqrtf((float)(sm->vx * sm->vx + sm->vy * sm->vy));
    float acc = sqrtf((float)(ax * ax + ay * ay));
    sm->current_speed = fix_from_float(spd);
    sm->current_acceleration = fix_from_float(acc);

    /* Contato e atrito físico com o corpo elástico */
    int dx = mouse_x - (pet_x + pet_radius);
    int dy = mouse_y - (pet_y + pet_radius);
    float dist = sqrtf((float)(dx * dx + dy * dy));

    if (dist < (float)pet_radius) {
        sm->is_contacting_body = true;
        /* Força de atrito proporcional à velocidade do cursor sobre o corpo */
        sm->friction_force_x = fix_from_float((float)sm->vx * 0.5f);
        sm->friction_force_y = fix_from_float((float)sm->vy * 0.5f);
    } else {
        sm->is_contacting_body = false;
        sm->friction_force_x = 0;
        sm->friction_force_y = 0;
    }
}

fix16_t sense_mouse_get_shake_intensity(const SenseMouse *sm) {
    if (!sm) return 0;
    /* Intensidade = aceleração + número de reversões acumuladas */
    fix16_t rev_factor = fix_from_int(sm->shake_reversals);
    return sat_add16(sm->current_acceleration >> 2, rev_factor);
}

bool sense_mouse_is_rubbing(const SenseMouse *sm) {
    return sm ? (sm->is_contacting_body && sm->current_speed > fix_from_float(1.0f) && sm->current_speed < fix_from_float(15.0f)) : false;
}
