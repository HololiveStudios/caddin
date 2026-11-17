#ifndef PET_SENSE_SCROLL_H
#define PET_SENSE_SCROLL_H

#include "../core/fixed.h"

typedef struct {
    fix16_t air_velocity; /* Corrente de ar acumulada (positiva para baixo, negativa para cima) */
    int decay_shift;
} SenseScroll;

void    sense_scroll_init(SenseScroll *ss);
void    sense_scroll_register(SenseScroll *ss, int wheel_delta);
void    sense_scroll_step(SenseScroll *ss);
fix16_t sense_scroll_get_wind(const SenseScroll *ss);

#endif /* PET_SENSE_SCROLL_H */
