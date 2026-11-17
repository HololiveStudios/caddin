#include "sense_scroll.h"

void sense_scroll_init(SenseScroll *ss) {
    if (!ss) return;
    ss->air_velocity = 0;
    ss->decay_shift = 4; /* Decai gradualmente a cada tick */
}

void sense_scroll_register(SenseScroll *ss, int wheel_delta) {
    if (!ss) return;
    /* Cada clique de rolagem adiciona um impulso de ar */
    fix16_t impulse = fix_from_float((float)wheel_delta * 0.4f);
    ss->air_velocity = sat_add16(ss->air_velocity, impulse);
}

void sense_scroll_step(SenseScroll *ss) {
    if (!ss) return;
    ss->air_velocity = fix_decay_shift(ss->air_velocity, ss->decay_shift);
}

fix16_t sense_scroll_get_wind(const SenseScroll *ss) {
    return ss ? ss->air_velocity : 0;
}
