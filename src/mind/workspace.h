#ifndef PET_MIND_WORKSPACE_H
#define PET_MIND_WORKSPACE_H

#include "../core/fixed.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    STIMULUS_NONE = 0,
    STIMULUS_MOUSE_TOUCH,
    STIMULUS_VIOLENT_SHAKE,
    STIMULUS_SCROLL_WIND,
    STIMULUS_WINDOW_DANGER,
    STIMULUS_SURPRISE_MISMATCH,
    STIMULUS_SPOKEN_WORD,
    STIMULUS_INTERNAL_FATIGUE
} StimulusType;

typedef struct {
    StimulusType type;
    fix16_t      intensity;
    fix16_t      salience;
    uint32_t     timestamp_tick;
} WorkspaceCandidate;

typedef struct {
    StimulusType current_focus;
    fix16_t      current_salience;
    uint32_t     focus_hold_ticks;
    uint32_t     max_hold_duration;

    /* Habituação: estímulos repetidos perdem saliência */
    uint8_t      habituation[16];
} GlobalWorkspace;

void         workspace_init(GlobalWorkspace *gw);
void         workspace_submit_candidate(GlobalWorkspace *gw, StimulusType type, fix16_t intensity, uint32_t current_tick);
void         workspace_step(GlobalWorkspace *gw, uint32_t current_tick);
StimulusType workspace_get_attended_focus(const GlobalWorkspace *gw);

#endif /* PET_MIND_WORKSPACE_H */
