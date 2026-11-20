#include "sense_fatigue.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

void sense_fatigue_init(SenseFatigue *sf) {
    if (!sf) return;
    memset(sf, 0, sizeof(*sf));
    sf->last_cursor_x = -1;
    sf->last_cursor_y = -1;
}

void sense_fatigue_register_key(SenseFatigue *sf, char key, double now_s) {
    if (!sf) return;
    sf->last_activity_t = now_s;
    sf->total_keys_count++;

    /* Backspace (8 ou 127) ou Delete */
    if (key == 8 || key == 127) {
        sf->backspace_count++;
    }
}

void sense_fatigue_register_mouse(SenseFatigue *sf, int cursor_x, int cursor_y, double now_s) {
    if (!sf) return;
    sf->last_activity_t = now_s;

    if (sf->last_cursor_x >= 0 && sf->last_cursor_y >= 0) {
        int dx = cursor_x - sf->last_cursor_x;
        int dy = cursor_y - sf->last_cursor_y;
        float dist = sqrtf((float)(dx * dx + dy * dy));

        /* Movimentos microrruidosos (hesitação/tremor 1..6 pixels) acumulam jitter */
        if (dist > 1.0f && dist < 6.0f) {
            sf->mouse_jitter_accum += 0.05f;
        }
    }
    sf->last_cursor_x = cursor_x;
    sf->last_cursor_y = cursor_y;
}

void sense_fatigue_update(SenseFatigue *sf, double dt, double now_s) {
    if (!sf) return;

    double idle_time = now_s - sf->last_activity_t;

    /* Se o usuário está em pausa (> 60 segundos ocioso), dissipa a fadiga */
    if (idle_time > 60.0) {
        sf->continuous_work_s = fmax(0.0, sf->continuous_work_s - dt * 2.5);
        sf->fatigue_level = fmaxf(0.0f, sf->fatigue_level - (float)dt * 0.02f);
        sf->mouse_jitter_accum = fmax(0.0, sf->mouse_jitter_accum - dt * 0.1);
        return;
    }

    /* Usuário ativo: acumula tempo contínuo de trabalho */
    sf->continuous_work_s += dt;

    /* Componente 1: Tempo Contínuo (2 horas ininterruptas = 7200s chegam a 0.6) */
    float time_factor = (float)(sf->continuous_work_s / 7200.0);
    if (time_factor > 0.65f) time_factor = 0.65f;

    /* Componente 2: Taxa de Erros de Digitação (mais de 15% de backspaces indica cansaço) */
    float error_factor = 0.0f;
    if (sf->total_keys_count > 50) {
        float err_ratio = (float)sf->backspace_count / (float)sf->total_keys_count;
        if (err_ratio > 0.12f) {
            error_factor = (err_ratio - 0.12f) * 1.8f;
            if (error_factor > 0.25f) error_factor = 0.25f;
        }
    }

    /* Componente 3: Jitter de Mouse */
    float jitter_factor = (float)fmin(0.2, sf->mouse_jitter_accum * 0.02);

    /* Integração dos fatores */
    float target_fatigue = time_factor + error_factor + jitter_factor;
    if (target_fatigue > 1.0f) target_fatigue = 1.0f;

    /* Suavização exponencial da fadiga */
    sf->fatigue_level += (target_fatigue - sf->fatigue_level) * 0.05f;

    /* Decaimento lento do acumulador de jitter */
    sf->mouse_jitter_accum = fmax(0.0, sf->mouse_jitter_accum - dt * 0.01);
}

float sense_fatigue_get_level(const SenseFatigue *sf) {
    return sf ? sf->fatigue_level : 0.0f;
}

bool sense_fatigue_is_exhausted(const SenseFatigue *sf) {
    return sf ? (sf->fatigue_level > 0.75f) : false;
}
