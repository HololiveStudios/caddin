#include "behavior.h"
#include <math.h>
#include <string.h>

/* ── Índices dos 24 Pools Motores (de network_data.h) ── */
enum {
    M_FACE_HAPPY = 0, M_FACE_SAD, M_FACE_SLEEP, M_FACE_SURPRISED,
    M_FACE_ANGRY, M_FACE_CURIOUS, M_FACE_PLAYFUL, M_FACE_NEUTRAL,
    M_FACE_WINK,
    M_CHIRP_SOUND, M_BEEP_PULSE, M_PURR_EFFECT,
    M_WANDER_LEFT, M_WANDER_RIGHT, M_JUMP_ACTION, M_DANCE_ACTION,
    M_SLEEP_ACTION, M_EAT_ACTION, M_CUDDLE_ACTION, M_ALERT_EMIT,
    M_NOD_HEAD, M_SHAKE_HEAD, M_THINK_EMIT, M_TAIL_WAG
};

void behavior_init(Behavior *b, Emotion *e) {
    if (b) memset(b, 0, sizeof(*b));
    if (e) memset(e, 0, sizeof(*e));
}

void behavior_update(Behavior *b, Drives *drives, ConditioningState *cond,
                     Brain *brain, double now_t, double dt) {
    (void)now_t; (void)cond;
    if (!b || !drives || !brain) return;

    b->state_timer += dt;

    /* ── Leitura dos motores neurais relevantes ── */
    double m_sleep   = brain_motor(brain, M_FACE_SLEEP);
    double m_surpr   = brain_motor(brain, M_FACE_SURPRISED);
    double m_curio   = brain_motor(brain, M_FACE_CURIOUS);
    double m_play    = brain_motor(brain, M_FACE_PLAYFUL);
    double m_eat     = brain_motor(brain, M_EAT_ACTION);
    double m_wander_l = brain_motor(brain, M_WANDER_LEFT);
    double m_wander_r = brain_motor(brain, M_WANDER_RIGHT);
    double m_sleep_act = brain_motor(brain, M_SLEEP_ACTION);

    /* ── Transições de Comportamento baseadas nos motores neurais ── */

    /* Sono: decidido pelo cérebro + drives */
    if (m_sleep_act > 0.35 || m_sleep > 0.40 || drives->energy < 0.12) {
        b->asleep = true;
        b->playful = false;
        b->wandering = false;
        b->curious = false;
    } else if (b->asleep && drives->energy > 0.40 && m_sleep < 0.20) {
        b->asleep = false;
    }

    if (!b->asleep) {
        /* Brincadeira: emergente do pool PLAYFUL */
        if (m_play > 0.30) {
            b->playful = true;
            b->curious = true;
        } else if (m_play < 0.15) {
            b->playful = false;
        }

        /* Susto: emergente do pool SURPRISED */
        if (m_surpr > 0.40) {
            b->startled = true;
        } else if (m_surpr < 0.20) {
            b->startled = false;
        }

        /* Curiosidade / vagar: emergente dos pools CURIOUS e WANDER */
        if (m_curio > 0.25 || m_wander_l > 0.25 || m_wander_r > 0.25) {
            b->wandering = true;
            b->curious = true;
        } else if (m_curio < 0.12) {
            b->wandering = false;
            b->curious = false;
        }

        /* Alimentação: emergente do pool EAT_ACTION */
        if (m_eat > 0.30 && drives->satiety < 0.50) {
            b->eating = true;
        } else {
            b->eating = false;
        }
    }
}

void behavior_update_emotion(Emotion *e, Behavior *b,
                            Brain *brain, double now_t, double dt) {
    (void)now_t;
    if (!e || !brain || !b) return;

    /* ── Leitura de TODOS os motores faciais/emocionais ── */
    double m_happy = brain_motor(brain, M_FACE_HAPPY);
    double m_sad   = brain_motor(brain, M_FACE_SAD);
    double m_sleep = brain_motor(brain, M_FACE_SLEEP);
    double m_surpr = brain_motor(brain, M_FACE_SURPRISED);
    double m_angry = brain_motor(brain, M_FACE_ANGRY);
    double m_curio = brain_motor(brain, M_FACE_CURIOUS);
    double m_play  = brain_motor(brain, M_FACE_PLAYFUL);
    double m_purr  = brain_motor(brain, M_PURR_EFFECT);
    double m_cuddle = brain_motor(brain, M_CUDDLE_ACTION);
    double m_alert = brain_motor(brain, M_ALERT_EMIT);

    /* ── Determina a emoção dominante (winner-takes-all suave) ── */
    /* Em vez de if/else rígido, calcula valência e arousal continuamente */

    double target_valence = 0.0;
    double target_arousal = 0.3;  /* baseline */

    /* Contribuições positivas */
    target_valence += m_happy * 0.9 + m_play * 0.7 + m_purr * 0.6 + m_cuddle * 0.5;
    target_arousal += m_play * 0.5 + m_alert * 0.6 + m_surpr * 0.7;

    /* Contribuições negativas */
    target_valence -= m_sad * 0.8 + m_angry * 0.7;
    target_arousal += m_angry * 0.5;

    /* Modulação do sono */
    if (b->asleep || m_sleep > 0.45) {
        target_arousal = 0.05;
        target_valence = 0.10;
    }

    /* Limitar */
    if (target_valence > 1.0) target_valence = 1.0;
    if (target_valence < -1.0) target_valence = -1.0;
    if (target_arousal > 1.0) target_arousal = 1.0;
    if (target_arousal < 0.0) target_arousal = 0.0;

    /* Suavização exponencial (não muda de humor instantaneamente) */
    double smooth = 1.0 - exp(-3.0 * dt);
    e->valence += (target_valence - e->valence) * smooth;
    e->arousal += (target_arousal - e->arousal) * smooth;

    /* ── Determina emoção discreta para o renderer (winner-takes-all) ── */
    if (b->asleep || m_sleep > 0.45) {
        e->current_emotion = 2;  /* dormindo */
    } else if (m_surpr > 0.40) {
        e->current_emotion = 3;  /* surpreso */
    } else if (m_angry > 0.35) {
        e->current_emotion = 4;  /* irritado */
    } else if (m_play > 0.30 || m_happy > 0.30) {
        e->current_emotion = 6;  /* brincalhão/feliz */
    } else if (m_curio > 0.25) {
        e->current_emotion = 5;  /* curioso */
    } else if (m_sad > 0.30) {
        e->current_emotion = 1;  /* triste */
    } else {
        e->current_emotion = 7;  /* neutro */
    }
}
