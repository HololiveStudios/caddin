#include "cpg.h"

#define CPG_WINDOW_SIZE 50 /* Janela de 50 ticks (100 ms a 2ms/tick) */
#define CRITICAL_SPIKE_RATIO_PER_TICK 35 /* > 35% de neurônios ativos por tick em média */

void cpg_init(CentralPatternGenerator *cpg) {
    if (!cpg) return;
    cpg->phase = 0;
    cpg->base_freq = 200; /* Frequência basal calma */
    cpg->current_depth = fix_from_float(0.12f);
    cpg->spike_window_accum = 0;
    cpg->window_ticks = 0;
    cpg->blackout_active = false;
    cpg->blackout_remaining_ticks = 0;
}

void cpg_step(CentralPatternGenerator *cpg, fix16_t arousal_stress, uint32_t current_spikes, uint32_t total_neurons) {
    if (!cpg) return;

    /* 1. Tratamento do estado de blackout (febre cognitiva) */
    if (cpg->blackout_active) {
        if (cpg->blackout_remaining_ticks > 0) {
            cpg->blackout_remaining_ticks--;
            return;
        } else {
            cpg->blackout_active = false;
            cpg->spike_window_accum = 0;
            cpg->window_ticks = 0;
        }
    }

    /* 2. Monitoramento de sobrecarga neural */
    cpg->spike_window_accum += current_spikes;
    cpg->window_ticks++;

    if (cpg->window_ticks >= CPG_WINDOW_SIZE) {
        if (total_neurons > 0) {
            uint32_t avg_spikes = cpg->spike_window_accum / CPG_WINDOW_SIZE;
            uint32_t spike_percent = (avg_spikes * 100) / total_neurons;
            if (spike_percent > CRITICAL_SPIKE_RATIO_PER_TICK) {
                /* Disparo de apagão protetor (febre cognitiva) */
                cpg->blackout_active = true;
                cpg->blackout_remaining_ticks = 40; /* ~80 ms de resfriamento */
            }
        }
        cpg->spike_window_accum = 0;
        cpg->window_ticks = 0;
    }

    /* 3. Modulação de ritmo respiratório por estresse e excitação */
    /* Quanto maior o estresse, mais rápida a frequência respiratória */
    uint16_t freq_boost = (uint16_t)((arousal_stress > 0) ? (arousal_stress << 1) : 0);
    uint16_t step_freq = cpg->base_freq + freq_boost;
    cpg->phase = (uint16_t)(cpg->phase + step_freq);
}

fix16_t cpg_get_breath_expansion(const CentralPatternGenerator *cpg) {
    if (!cpg || cpg->blackout_active) return 0;

    /* Converte fase [0..65535] em onda senoidal suave [-1.0, 1.0] em Q8.8 */
    /* Usando aproximação triangular filtrada */
    int32_t p = (int32_t)(cpg->phase >> 8); /* [0..255] */
    int32_t wave;
    if (p < 128) {
        wave = (p - 64) * 4; /* [-256..256] */
    } else {
        wave = (192 - p) * 4; /* [256..-256] */
    }

    fix16_t normalized_wave = (fix16_t)wave;
    return fix_mul(normalized_wave, cpg->current_depth);
}

bool cpg_is_in_blackout(const CentralPatternGenerator *cpg) {
    return cpg ? cpg->blackout_active : false;
}
