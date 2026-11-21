#include "autonomy.h"
#ifndef CADDIN_HEADLESS
#  include "x11_cursor.h"
#endif
#include "config.h"
#include "neuro.h"
#include <math.h>
#include <stdlib.h>

/* ── Índices dos motores relevantes para autonomia ── */
enum {
    M_WANDER_LEFT  = 12,
    M_WANDER_RIGHT = 13,
    M_JUMP_ACTION  = 14,
    M_DANCE_ACTION = 15
};

void autonomy_init(AutonomyState *au) {
    au->decision_timer   = 4.0;
    au->observe_timer    = 0.0;
    au->last_mouse_x     = 0;
    au->last_mouse_y     = 0;
    au->mouse_activity_t = 0.0;
    au->smooth_look_x    = 0.f;
    au->smooth_look_y    = 0.f;
}

void autonomy_update(AutonomyState *au, double dt, PetWindow *win,
                     SoftBody *sb, EmotionMachine *em, const Brain *brain) {
    if (!win || !sb || !em) return;

    /* Não toma ações autônomas enquanto o usuário está arrastando o pet */
    if (window_is_dragging(win)) {
        au->decision_timer = 2.0;
        return;
    }

    /* ── 1. Rastreia o ponteiro do usuário na tela (Vendo o que você faz) ── */
    int scr_mx = 0, scr_my = 0;
#ifdef CADDIN_HEADLESS
    bool has_cursor = false;
#else
    bool has_cursor = x11_get_screen_cursor(&scr_mx, &scr_my);
#endif

    int win_x = 0, win_y = 0;
    window_get_pos(win, &win_x, &win_y);

    int pet_cx = win_x + WINDOW_W / 2;
    int pet_cy = win_y + WINDOW_H / 2;

    if (has_cursor) {
        /* Detecta se o usuário está mexendo no mouse */
        double m_dist = hypot((double)(scr_mx - au->last_mouse_x), (double)(scr_my - au->last_mouse_y));
        if (m_dist > 4.0) {
            au->mouse_activity_t = 1.5; /* Usuário ativo */
        } else if (au->mouse_activity_t > 0.0) {
            au->mouse_activity_t -= dt;
        }
        au->last_mouse_x = scr_mx;
        au->last_mouse_y = scr_my;

        /* Direção do olhar para o mouse na tela */
        float dx = (float)(scr_mx - pet_cx);
        float dy = (float)(scr_my - pet_cy);

        float target_look_x = dx / 450.0f;
        float target_look_y = dy / 350.0f;
        if (target_look_x < -0.85f) target_look_x = -0.85f;
        if (target_look_x >  0.85f) target_look_x =  0.85f;
        if (target_look_y < -0.65f) target_look_y = -0.65f;
        if (target_look_y >  0.65f) target_look_y =  0.65f;

        /* Suavização lerp do olhar */
        float look_speed = (au->mouse_activity_t > 0.0) ? 9.0f : 4.0f;
        float alpha = (float)(1.0 - exp(-(double)look_speed * dt));
        au->smooth_look_x += (target_look_x - au->smooth_look_x) * alpha;
        au->smooth_look_y += (target_look_y - au->smooth_look_y) * alpha;

        em->look_x = au->smooth_look_x;
        em->look_y = au->smooth_look_y;
    }

    /* ── 2. Passeio Autônomo Guiado pelos Motores Neurais ── */
    /* Só toma decisões de passeio quando o pet está em repouso no chão */
    if (window_get_speed(win) > 20.0) {
        return;
    }

    au->decision_timer -= dt;
    if (au->decision_timer <= 0.0) {
        /* Intervalo para próxima ação: 2.5 a 5.5s */
        au->decision_timer = 2.5 + 3.0 * ((double)rand() / RAND_MAX);

        int scr_x, scr_y, scr_w, scr_h;
        window_get_workarea(&scr_x, &scr_y, &scr_w, &scr_h);

        int min_x = scr_x + 10;
        int max_x = scr_x + scr_w - WINDOW_W - 10;

        /* ── Decisão emergente baseada nos motores neurais ── */
        double m_wander_l = brain ? brain_motor(brain, M_WANDER_LEFT) : 0.0;
        double m_wander_r = brain ? brain_motor(brain, M_WANDER_RIGHT) : 0.0;
        double m_dance    = brain ? brain_motor(brain, M_DANCE_ACTION) : 0.0;

        /* Se algum motor de locomoção estiver ativo, ou por probabilidade basal */
        bool wants_wander = (m_wander_l > 0.18 || m_wander_r > 0.18);
        int roll = rand() % 100;

        if (wants_wander || roll < 50) {
            /* Ação A: Pulinho para passear (hop walk) */
            int dir = 0;
            if (win_x <= min_x + 30) {
                dir = 1; /* Perto da borda esquerda -> vai pra direita */
            } else if (win_x >= max_x - 30) {
                dir = -1; /* Perto da borda direita -> vai pra esquerda */
            } else if (fabs(m_wander_r - m_wander_l) > 0.06) {
                /* Direção determinada pelo neurônio motor dominante */
                dir = (m_wander_r > m_wander_l) ? 1 : -1;
            } else if (has_cursor && au->mouse_activity_t > 0.0) {
                /* Curiosidade: tende a caminhar em direção onde você está mexendo o mouse */
                dir = (scr_mx > pet_cx) ? 1 : -1;
            } else {
                dir = (rand() % 2 == 0) ? 1 : -1;
            }

            double speed_boost = fmax(m_wander_l, m_wander_r) * 100.0;
            double hop_power_x = (double)(130 + rand() % 60 + (int)speed_boost) * (double)dir;
            double hop_power_y = -(240.0 + (double)(rand() % 70));

            window_apply_impulse(win, hop_power_x, hop_power_y);
            softbody_stretch(sb, 0.12f);

        } else if (m_dance > 0.25 || roll < 80) {
            /* Ação B: Inspecionar o que o usuário está fazendo / dançar */
            softbody_jiggle(sb, 0.10f);

        } else {
            /* Ação C: Suspiro / relaxamento / ronronar */
            softbody_purr(sb, 0.8f);
        }
    }
}
