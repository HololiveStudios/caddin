#include "behavior_dance.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void behavior_dance_init(BehaviorDance *d) {
    if (!d) return;
    d->is_dancing = false;
    d->style = DANCE_STYLE_BOUNCE;
    d->dance_timer = 0.0f;
    d->style_timer = 0.0f;
    d->beat_counter = 0;
    d->sway_vel_x = 0.0f;
    d->bounce_y = 0.0f;
    d->should_chirp = false;
}

bool behavior_dance_update(BehaviorDance *d, float dt,
                           bool is_music, float bpm, float beat_phase,
                           bool is_on_beat, float beat_intensity,
                           SoftBody *sb, float *out_dx, float *out_bob_y) {
    if (!d) return false;
    if (out_dx) *out_dx = 0.0f;
    if (out_bob_y) *out_bob_y = 0.0f;
    d->should_chirp = false;

    if (!is_music) {
        if (d->is_dancing) {
            d->dance_timer += dt;
            /* Suavização final da dança ao cessar a música */
            if (d->dance_timer > 0.8f) {
                d->is_dancing = false;
                d->style = DANCE_STYLE_IDLE;
                d->dance_timer = 0.0f;
            }
        }
        return false;
    }

    d->is_dancing = true;
    d->dance_timer += dt;
    d->style_timer += dt;

    /* Alterna estilos de dança a cada 16 batidas (~8 segundos) */
    if (is_on_beat) {
        d->beat_counter++;
        if (d->beat_counter % 16 == 0) {
            d->style = (DanceStyle)((d->style % 4) + 1);
            /* Celebração rítmica com trilo/chirp */
            d->should_chirp = true;
        }

        /* Resposta física imediata do corpo mole no impacto da batida */
        if (sb) {
            float squish_power = 0.22f * (beat_intensity > 0.3f ? beat_intensity : 0.6f);
            softbody_squish(sb, squish_power);

            /* No início de cada compasso de 4 batidas, jiggle festivo */
            if (d->beat_counter % 4 == 0) {
                softbody_jiggle(sb, 0.12f * beat_intensity);
            }
        }
    }

    /* Modulação contínua ao longo da fase da batida [0.0 .. 1.0] */
    float phase_rad = beat_phase * 2.0f * (float)M_PI;
    float bounce = 0.0f;
    float sway = 0.0f;

    switch (d->style) {
    case DANCE_STYLE_BOUNCE:
        /* Salto parabólico elástico: atinge o topo no meio da batida e quica no chão */
        bounce = -fabsf(sinf(beat_phase * (float)M_PI)) * 16.0f;
        sway = sinf(phase_rad * 0.5f) * 8.0f * dt;
        break;

    case DANCE_STYLE_SWAY:
        /* Balanço pendular cadenciado esquerda-direita */
        sway = cosf(phase_rad * 0.5f) * 26.0f * dt;
        bounce = -fabsf(sinf(beat_phase * (float)M_PI)) * 8.0f;
        if (sb && fabsf(sinf(phase_rad)) > 0.8f) {
            softbody_stretch(sb, 0.04f);
        }
        break;

    case DANCE_STYLE_WIGGLE:
        /* Vibração rápida e jiggle de alta energia */
        bounce = -fabsf(sinf(beat_phase * (float)M_PI)) * 10.0f;
        sway = sinf(phase_rad * 2.0f) * 14.0f * dt;
        if (sb && is_on_beat) {
            softbody_jiggle(sb, 0.16f);
        }
        break;

    case DANCE_STYLE_GROOVE:
    default:
        /* Combinação fluida de balanço e salto */
        bounce = -fabsf(sinf(beat_phase * (float)M_PI)) * 14.0f;
        sway = sinf(phase_rad * 0.25f) * 20.0f * dt;
        break;
    }

    /* Escala com o andamento (BPM mais rápido = movimentos mais ágeis) */
    float bpm_scale = bpm / 120.0f;
    if (bpm_scale < 0.7f) bpm_scale = 0.7f;
    if (bpm_scale > 1.5f) bpm_scale = 1.5f;

    if (out_dx) *out_dx = sway * bpm_scale;
    if (out_bob_y) *out_bob_y = bounce;

    return true;
}
