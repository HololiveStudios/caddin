#include "sense_windows.h"
#include <stdlib.h>

void sense_windows_init(SenseWindows *sw) {
    if (!sw) return;
    sw->active_win_x = 0;
    sw->active_win_y = 0;
    sw->active_win_w = 0;
    sw->active_win_h = 0;
    sw->last_focused_win_id = 0;
    sw->focus_switched = false;
}

void sense_windows_update(SenseWindows *sw, uint64_t win_id, int x, int y, int w, int h) {
    if (!sw) return;

    if (win_id != 0 && win_id != sw->last_focused_win_id) {
        sw->focus_switched = true;
        sw->last_focused_win_id = win_id;
    }

    sw->active_win_x = x;
    sw->active_win_y = y;
    sw->active_win_w = w;
    sw->active_win_h = h;
}

bool sense_windows_is_on_top_edge(const SenseWindows *sw, int pet_x, int pet_y, int tolerance_px) {
    if (!sw || sw->active_win_w == 0) return false;

    /* Verifica se o pet está horizontalmente sobre a barra de título */
    if (pet_x >= sw->active_win_x && pet_x <= (sw->active_win_x + sw->active_win_w)) {
        /* Verifica se a coordenada vertical está próxima do topo da janela */
        int dy = abs(pet_y - sw->active_win_y);
        return (dy <= tolerance_px);
    }
    return false;
}

bool sense_windows_is_near_danger_zone(const SenseWindows *sw, int pet_x, int pet_y, int danger_radius_px) {
    if (!sw || sw->active_win_w == 0) return false;

    /* Quina superior direita: botão fechar */
    int close_button_x = sw->active_win_x + sw->active_win_w - 20;
    int close_button_y = sw->active_win_y + 15;

    int dx = pet_x - close_button_x;
    int dy = pet_y - close_button_y;
    return ((dx * dx + dy * dy) <= (danger_radius_px * danger_radius_px));
}

bool sense_windows_did_switch_focus(SenseWindows *sw) {
    if (!sw) return false;
    bool switched = sw->focus_switched;
    sw->focus_switched = false;
    return switched;
}
