#include "behavior.h"
#include <math.h>
#include <string.h>

void behavior_init(Behavior *b, Emotion *e, FaceState *f) {
    if (b) memset(b, 0, sizeof(*b));
    if (e) memset(e, 0, sizeof(*e));
    if (f) {
        memset(f, 0, sizeof(*f));
        f->face_str = "( o _ o )";
        f->status_msg = "Pestinha esta observando o terminal...";
        f->action_msg_timer = 0.0;
        f->action_msg[0] = '\0';
        f->last_key_pressed = 0;
    }
}

void behavior_update(Behavior *b, Drives *drives, ConditioningState *cond,
                     Brain *brain, double now_t, double dt) {
    (void)now_t; (void)cond;
    if (!b || !drives || !brain) return;

    b->state_timer += dt;

    /* Leitura dos neurônios motores principais */
    double m_sleep = brain_motor(brain, 2);
    double m_surpr = brain_motor(brain, 3);
    double m_curio = brain_motor(brain, 5);
    double m_play  = brain_motor(brain, 6);

    /* Transições de Comportamento */
    if (drives->energy < 0.15 || m_sleep > 0.40) {
        b->asleep = true;
        b->playful = false;
        b->wandering = false;
    } else if (drives->energy > 0.40 && b->asleep) {
        b->asleep = false;
    }

    if (!b->asleep) {
        if (m_play > 0.35 || drives->curiosity > 0.70) {
            b->playful = true;
            b->curious = true;
        } else {
            b->playful = false;
        }

        if (m_surpr > 0.45 || drives->stress > 0.75) {
            b->startled = true;
        } else {
            b->startled = false;
        }

        if (m_curio > 0.30) {
            b->wandering = true;
        }
    }
}

void behavior_update_emotion(Emotion *e, Behavior *b, FaceState *f,
                            Brain *brain, double now_t, double dt) {
    (void)now_t;
    if (!e || !f || !brain || !b) return;

    /* Atualiza o cronômetro da mensagem de ação */
    if (f->action_msg_timer > 0.0) {
        f->action_msg_timer -= dt;
        if (f->action_msg_timer < 0.0) {
            f->action_msg_timer = 0.0;
        }
    }

    double m_happy = brain_motor(brain, 0);
    double m_sad   = brain_motor(brain, 1);
    double m_sleep = brain_motor(brain, 2);
    double m_surpr = brain_motor(brain, 3);
    double m_angry = brain_motor(brain, 4);
    double m_curio = brain_motor(brain, 5);
    double m_play  = brain_motor(brain, 6);

    if (b->asleep || m_sleep > 0.45) {
        f->current_face = 2;
        f->face_str = "( - _ - ) zZ";
        f->status_msg = "Pestinha esta dormindo suavemente...";
        e->arousal = 0.05;
        e->valence = 0.10;
    } else if (b->startled || m_surpr > 0.40) {
        f->current_face = 3;
        f->face_str = "( O _ O ) !";
        f->status_msg = "Pestinha esta em estado de alerta!";
        e->arousal = 0.90;
        e->valence = -0.20;
    } else if (m_angry > 0.38) {
        f->current_face = 4;
        f->face_str = "( > _ < )";
        f->status_msg = "Pestinha esta irritado!";
        e->arousal = 0.85;
        e->valence = -0.60;
    } else if (m_play > 0.35 || m_happy > 0.35) {
        f->current_face = 6;
        f->face_str = "( ≧ ▽ ≦ )";
        f->status_msg = "Pestinha esta super animado e feliz!";
        e->arousal = 0.75;
        e->valence = 0.85;
    } else if (m_curio > 0.30) {
        f->current_face = 5;
        f->face_str = "( ⊙ _ ☉ ) ?";
        f->status_msg = "Pestinha esta curioso explorando o terminal...";
        e->arousal = 0.50;
        e->valence = 0.40;
    } else if (m_sad > 0.35) {
        f->current_face = 1;
        f->face_str = "( T _ T )";
        f->status_msg = "Pestinha esta carente...";
        e->arousal = 0.20;
        e->valence = -0.70;
    } else {
        f->current_face = 7;
        f->face_str = "( o _ o )";
        f->status_msg = "Pestinha esta observando o terminal.";
        e->arousal = 0.30;
        e->valence = 0.20;
    }
}
