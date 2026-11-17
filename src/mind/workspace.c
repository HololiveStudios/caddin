#include "workspace.h"

void workspace_init(GlobalWorkspace *gw) {
    if (!gw) return;
    gw->current_focus = STIMULUS_NONE;
    gw->current_salience = 0;
    gw->focus_hold_ticks = 0;
    gw->max_hold_duration = 100; /* ~200 ms a 2ms/tick */

    for (int i = 0; i < 16; ++i) {
        gw->habituation[i] = 0;
    }
}

void workspace_submit_candidate(GlobalWorkspace *gw, StimulusType type, fix16_t intensity, uint32_t current_tick) {
    if (!gw || type == STIMULUS_NONE || (int)type >= 16) return;
    (void)current_tick;

    /* Cálculo de saliência considerando habituação */
    /* saliência = intensidade * (1.0 - habituação) */
    uint8_t hab = gw->habituation[(int)type];
    fix16_t hab_factor = fix_from_float(1.0f - ((float)hab / 256.0f));
    fix16_t salience = fix_mul(intensity, hab_factor);

    /* Aumentar habituação pela exposição contínua ao estímulo */
    if (gw->habituation[(int)type] < 220) {
        gw->habituation[(int)type] += 25;
    }

    /* Competição por atenção (WTA - Winner Take All com histerese) */
    if (salience > gw->current_salience || gw->focus_hold_ticks >= gw->max_hold_duration) {
        gw->current_focus = type;
        gw->current_salience = salience;
        gw->focus_hold_ticks = 0;
    }
}

void workspace_step(GlobalWorkspace *gw, uint32_t current_tick) {
    if (!gw) return;
    (void)current_tick;

    if (gw->current_focus != STIMULUS_NONE) {
        gw->focus_hold_ticks++;
        /* Decaimento da saliência do foco atual */
        gw->current_salience = fix_decay_shift(gw->current_salience, 4);

        if (gw->focus_hold_ticks >= gw->max_hold_duration) {
            gw->current_focus = STIMULUS_NONE;
            gw->current_salience = 0;
            gw->focus_hold_ticks = 0;
        }
    }

    /* Recuperação gradual da habituação (desabituação) */
    for (int i = 0; i < 16; ++i) {
        if (gw->habituation[i] > 0) {
            gw->habituation[i]--;
        }
    }
}

StimulusType workspace_get_attended_focus(const GlobalWorkspace *gw) {
    return gw ? gw->current_focus : STIMULUS_NONE;
}
