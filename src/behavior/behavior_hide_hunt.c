#include "behavior_hide_hunt.h"
#include <math.h>

void behavior_hide_hunt_init(BehaviorHideHunt *hh) {
    if (!hh) return;
    hh->state = HIDE_HUNT_IDLE;
    hh->state_timer = 0.0f;
    hh->target_x = 0.0f;
    hh->target_y = 0.0f;
    hh->stalk_speed = 35.0f;
    hh->is_hidden = false;
    hh->last_eval_tick = 0;
}

void behavior_hide_hunt_update(BehaviorHideHunt *hh, float dt,
                               float pet_x, float pet_y,
                               float mouse_x, float mouse_y,
                               int win_x, int win_y, int win_w, int win_h,
                               float stress, float energy, float curiosity,
                               float *out_dx, float *out_dy) {
    if (!hh) return;
    if (out_dx) *out_dx = 0.0f;
    if (out_dy) *out_dy = 0.0f;

    hh->state_timer += dt;

    /* Transições de Estado Baseadas em Pools Emocionais */
    if (hh->state == HIDE_HUNT_IDLE) {
        /* Estresse alto (> 0.65) ativa instinto de fuga e se esconder */
        if (stress > 0.65f) {
            hh->state = HIDE_HUNT_HIDING;
            hh->state_timer = 0.0f;
            /* Procura a borda da janela ativa mais próxima */
            if (win_w > 50 && win_h > 50) {
                float dist_left = fabsf(pet_x - (float)win_x);
                float dist_right = fabsf(pet_x - (float)(win_x + win_w));
                if (dist_left < dist_right) {
                    hh->target_x = (float)win_x - 90.0f; /* 80% escondido além da borda esquerda */
                } else {
                    hh->target_x = (float)(win_x + win_w) - 30.0f; /* além da borda direita */
                }
                hh->target_y = pet_y;
            } else {
                /* Borda da tela como refúgio */
                hh->target_x = (pet_x < 500.0f) ? -60.0f : 1850.0f;
                hh->target_y = pet_y;
            }
        }
        /* Bom humor (baixo estresse, energia > 0.6, curiosidade > 0.5) ativa caça do cursor */
        else if (stress < 0.25f && energy > 0.60f && curiosity > 0.50f) {
            float dist_mouse = hypotf(mouse_x - pet_x, mouse_y - pet_y);
            if (dist_mouse > 120.0f && dist_mouse < 500.0f) {
                hh->state = HIDE_HUNT_STALKING;
                hh->state_timer = 0.0f;
                hh->target_x = mouse_x;
                hh->target_y = mouse_y;
            }
        }
    }
    else if (hh->state == HIDE_HUNT_HIDING) {
        float dx = hh->target_x - pet_x;
        float dy = hh->target_y - pet_y;
        float dist = hypotf(dx, dy);

        if (dist > 8.0f) {
            float move = 180.0f * dt; /* corrida veloz para a borda */
            if (move > dist) move = dist;
            if (out_dx) *out_dx = (dx / dist) * move;
            if (out_dy) *out_dy = (dy / dist) * move;
            hh->is_hidden = false;
        } else {
            hh->is_hidden = true;
            /* Se acalmou ou passou tempo suficiente escondido, passa a espiar */
            if (hh->state_timer > 3.0f || stress < 0.40f) {
                hh->state = HIDE_HUNT_PEEKING;
                hh->state_timer = 0.0f;
            }
        }
    }
    else if (hh->state == HIDE_HUNT_PEEKING) {
        /* Espiando timidamente: move-se levemente 15 px para fora */
        if (hh->state_timer < 2.0f) {
            if (out_dx) *out_dx = (hh->target_x < pet_x) ? 10.0f * dt : -10.0f * dt;
        } else {
            hh->state = HIDE_HUNT_IDLE;
            hh->state_timer = 0.0f;
            hh->is_hidden = false;
        }
    }
    else if (hh->state == HIDE_HUNT_STALKING) {
        /* Aproximação furtiva e lenta do cursor */
        float dx = mouse_x - pet_x;
        float dy = mouse_y - pet_y;
        float dist = hypotf(dx, dy);

        if (dist > 70.0f) {
            float move = hh->stalk_speed * dt;
            if (out_dx) *out_dx = (dx / dist) * move;
            if (out_dy) *out_dy = (dy / dist) * move;
        } else if (dist <= 70.0f) {
            /* Perto o suficiente para o bote */
            hh->state = HIDE_HUNT_POUNCING;
            hh->state_timer = 0.0f;
        }

        /* Se o usuário mover o mouse rápido demais ou se estressar, aborta */
        if (hh->state_timer > 6.0f || stress > 0.4f) {
            hh->state = HIDE_HUNT_IDLE;
        }
    }
    else if (hh->state == HIDE_HUNT_POUNCING) {
        /* Bote lúdico rápido */
        float dx = mouse_x - pet_x;
        float dy = mouse_y - pet_y;
        float dist = hypotf(dx, dy);

        if (hh->state_timer < 0.25f && dist > 5.0f) {
            float move = 280.0f * dt;
            if (out_dx) *out_dx = (dx / dist) * move;
            if (out_dy) *out_dy = (dy / dist) * move;
        } else {
            hh->state = HIDE_HUNT_IDLE;
            hh->state_timer = 0.0f;
        }
    }
}
