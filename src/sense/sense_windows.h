#ifndef PET_SENSE_WINDOWS_H
#define PET_SENSE_WINDOWS_H

#include "../core/fixed.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int active_win_x;
    int active_win_y;
    int active_win_w;
    int active_win_h;
    uint64_t last_focused_win_id;
    bool focus_switched;
} SenseWindows;

void sense_windows_init(SenseWindows *sw);
void sense_windows_update(SenseWindows *sw, uint64_t win_id, int x, int y, int w, int h);
bool sense_windows_is_on_top_edge(const SenseWindows *sw, int pet_x, int pet_y, int tolerance_px);
bool sense_windows_is_near_danger_zone(const SenseWindows *sw, int pet_x, int pet_y, int danger_radius_px);
bool sense_windows_did_switch_focus(SenseWindows *sw);

#endif /* PET_SENSE_WINDOWS_H */
