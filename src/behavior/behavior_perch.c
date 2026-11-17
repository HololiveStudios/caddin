#include "behavior_perch.h"
#include <math.h>

void behavior_perch_init(BehaviorPerch *p) {
    if (!p) return;
    p->state = PERCH_STATE_FREE;
    p->perch_x = 0.0f;
    p->perch_y = 0.0f;
    p->hold_timer = 0.0f;
    p->is_perched = false;
}

bool behavior_perch_update(BehaviorPerch *p, float dt,
                           float pet_x, float pet_y,
                           int win_x, int win_y, int win_w, int win_h,
                           float energy, float stress,
                           float *out_dx, float *out_dy) {
    if (!p) return false;
    if (out_dx) *out_dx = 0.0f;
    if (out_dy) *out_dy = 0.0f;

    /* Janela inválida ou minimizada: solta o poleiro imediatamente */
    if (win_w < 100 || win_h < 100 || win_x < -2000 || win_y < -2000) {
        p->state = PERCH_STATE_FREE;
        p->is_perched = false;
        return false;
    }

    /* Posição ideal do poleiro: logo acima da borda superior da janela ativa */
    float target_y = (float)win_y - 110.0f; /* descanso da base do mascote no topo */
    float min_x = (float)win_x + 30.0f;
    float max_x = (float)(win_x + win_w) - 150.0f;

    if (p->state == PERCH_STATE_FREE) {
        /* Se estiver relaxado (estresse baixo < 0.35, energia moderada < 0.70)
         * e geograficamente perto do topo da janela (distância vertical < 120px) */
        if (stress < 0.35f && energy < 0.70f) {
            float dist_y = fabsf(pet_y - target_y);
            if (dist_y < 120.0f && pet_x >= min_x && pet_x <= max_x) {
                p->state = PERCH_STATE_APPROACHING;
                p->perch_x = pet_x;
                p->perch_y = target_y;
                p->hold_timer = 0.0f;
            }
        }
    }
    else if (p->state == PERCH_STATE_APPROACHING) {
        float dy = target_y - pet_y;
        if (fabsf(dy) > 3.0f) {
            float move = 90.0f * dt;
            if (out_dy) *out_dy = (dy > 0 ? 1.0f : -1.0f) * (fabsf(dy) < move ? fabsf(dy) : move);
        } else {
            p->state = PERCH_STATE_PERCHED;
            p->is_perched = true;
            p->hold_timer = 0.0f;
        }
    }
    else if (p->state == PERCH_STATE_PERCHED) {
        p->hold_timer += dt;
        p->is_perched = true;

        /* Acompanha suavemente o topo da janela caso ela seja arrastada */
        float dy = target_y - pet_y;
        if (fabsf(dy) > 2.0f && out_dy) {
            *out_dy = dy * 0.2f;
        }

        /* Sai do poleiro se o mascote se estressar, acordar agitado ou após 15 a 30s de descanso */
        if (stress > 0.45f || energy > 0.85f || p->hold_timer > 25.0f) {
            p->state = PERCH_STATE_LEAVING;
            p->hold_timer = 0.0f;
        }
    }
    else if (p->state == PERCH_STATE_LEAVING) {
        p->hold_timer += dt;
        p->is_perched = false;
        /* Pequeno impulso de voo/salto para cima */
        if (out_dy) *out_dy = -60.0f * dt;
        if (p->hold_timer > 0.6f) {
            p->state = PERCH_STATE_FREE;
            p->hold_timer = 0.0f;
        }
    }

    return p->is_perched;
}
