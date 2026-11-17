#include "emotions.h"
#include "window.h"
#include "neuro.h"
#include <math.h>
#include <stdlib.h>

#define PI 3.14159265358979323846

void emot_init(EmotionMachine *em) {
    em->current           = EMO_IDLE;
    em->previous          = EMO_IDLE;
    em->state_timer       = 0.0;
    em->cooldown_timer    = 0.0;
    em->last_drag_vx      = 0.0;
    em->direction_changes = 0;
    em->shake_window_t    = 0.0;
    em->shake_duration    = 0.0;
    em->gentle_drag_t     = 0.0;
    em->breath_phase      = 0.0;
    em->breath_scale      = 1.0f;
    em->blink_timer       = 3.0;
    em->blink_phase       = 0.0;
    em->blinking          = 0;
    em->blink_lid         = 0.f;
    em->look_x            = 0.f;
    em->look_y            = 0.f;
    em->look_timer        = 2.0;
}

void emot_trigger(EmotionMachine *em, PetEmotion new_emo, double duration) {
    if (em->current != new_emo) {
        em->previous    = em->current;
        em->current     = new_emo;
        em->state_timer = 0.0;
    }
    em->cooldown_timer = duration;
}

void emot_update(EmotionMachine *em, double dt,
                 bool is_dragging, double drag_speed, double drag_vx,
                 bool was_clicked, bool was_bumped) {
    (void)was_clicked;
    (void)was_bumped;
    em->state_timer += dt;

    /* ── Arraste e Detector de Agitação Física (Shake) ── */
    if (is_dragging) {
        /* Enquanto arrasta, desativa o piscar aleatório para não bugar a expressão */
        em->blinking  = 0;
        em->blink_lid = 0.f;

        em->shake_window_t += dt;

        if (fabs(drag_vx) > 150.0) {
            if ((drag_vx > 0 && em->last_drag_vx < -70.0) ||
                (drag_vx < 0 && em->last_drag_vx >  70.0)) {
                em->direction_changes++;
            }
            em->last_drag_vx = drag_vx;
        }

        if (em->direction_changes >= 4 && em->shake_window_t < 1.6) {
            em->shake_duration += dt;
            if (em->shake_duration > 1.2) {
                emot_trigger(em, EMO_DIZZY, 2.8);
            }
            if (em->shake_duration > 2.4) {
                emot_trigger(em, EMO_ANNOYED, 3.2);
            }
        }

        if (em->shake_window_t >= 1.6) {
            em->direction_changes = 0;
            em->shake_window_t = 0.0;
        }

        if (em->current != EMO_DIZZY && em->current != EMO_ANNOYED) {
            if (drag_speed > 550.0) {
                em->gentle_drag_t = 0.0;
                emot_trigger(em, EMO_SURPRISED, 1.5);
            } else if (drag_speed < 180.0 && drag_speed > 4.0) {
                /* Arraste suave sustentado: ativa carinho sem piscar/flicker */
                em->gentle_drag_t += dt;
                if (em->gentle_drag_t > 0.25) {
                    emot_trigger(em, EMO_AFFECTION, 2.0);
                }
            } else {
                em->gentle_drag_t = 0.0;
                if (em->current != EMO_AFFECTION) {
                    emot_trigger(em, EMO_CURIOUS, 1.4);
                }
            }
        }
    } else {
        em->shake_duration    = 0.0;
        em->direction_changes = 0;
        em->shake_window_t    = 0.0;
        em->gentle_drag_t     = 0.0;
    }

    /* ── 5. Cooldown suave para IDLE ── */
    if (em->current != EMO_IDLE) {
        em->cooldown_timer -= dt;
        if (em->cooldown_timer <= 0.0) {
            em->current = EMO_IDLE;
        }
    }

    /* ── 6. Comportamento IDLE: Respiração e Piscar Natural ── */
    em->breath_phase += dt * 1.0;
    em->breath_scale = 1.0f + 0.018f * (float)sin(em->breath_phase);

    /* Piscar natural só acontece se não estiver sendo arrastado e nem em afeto/tontura */
    if (!is_dragging && em->current != EMO_DIZZY && em->current != EMO_AFFECTION) {
        if (em->blinking) {
            em->blink_phase += dt * PI / 0.12;
            if (em->blink_phase >= PI) {
                em->blinking    = 0;
                em->blink_phase = 0.0;
                em->blink_lid   = 0.f;
            } else {
                em->blink_lid = (float)sin(em->blink_phase);
            }
        } else {
            em->blink_timer -= dt;
            if (em->blink_timer <= 0.0) {
                em->blinking    = 1;
                em->blink_phase = 0.0;
                em->blink_timer = 2.8 + 3.2 * ((double)rand() / RAND_MAX);
            }
        }
    } else {
        em->blinking  = 0;
        em->blink_lid = 0.f;
    }
}

void emot_sync_brain(EmotionMachine *em, const Brain *brain) {
    if (!em || !brain) return;

    /* Tontura mecânica (sacudida contínua) tem prioridade física temporária */
    if (em->current == EMO_DIZZY && em->cooldown_timer > 0.0) return;

    /* ── Motores faciais da SNN ── */
    double m_happy = brain_motor(brain, 0);  /* FACE_HAPPY */
    double m_sad   = brain_motor(brain, 1);  /* FACE_SAD */
    double m_sleep = brain_motor(brain, 2);  /* FACE_SLEEP */
    double m_surpr = brain_motor(brain, 3);  /* FACE_SURPRISED */
    double m_angry = brain_motor(brain, 4);  /* FACE_ANGRY */
    double m_curio = brain_motor(brain, 5);  /* FACE_CURIOUS */
    double m_play  = brain_motor(brain, 6);  /* FACE_PLAYFUL */
    double m_purr  = brain_motor(brain, 11); /* PURR_EFFECT */

    if (m_angry > 0.35) {
        emot_trigger(em, EMO_ANNOYED, 0.4);
    } else if (m_surpr > 0.38) {
        emot_trigger(em, EMO_SURPRISED, 0.4);
    } else if (m_purr > 0.25 || (m_play > 0.30 && m_happy > 0.30)) {
        emot_trigger(em, EMO_AFFECTION, 0.5);
    } else if (m_curio > 0.25) {
        emot_trigger(em, EMO_CURIOUS, 0.4);
    }
}
