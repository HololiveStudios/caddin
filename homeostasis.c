#include "homeostasis.h"
#include <math.h>
#include <string.h>

void homeostasis_init(Drives *drives) {
    if (!drives) return;
    drives->energy = 0.85;
    drives->stress = 0.15;
    drives->curiosity = 0.60;
    drives->satiety = 0.75;
    drives->affection = 0.50;
}

void homeostasis_update(Drives *d, Behavior *b, double dt, int moving,
                        double arousal, const double inputs[PESTINHA_CHANNELS]) {
    if (!d) return;
    (void)inputs;

    /* A. Decaimento passivo de energia */
    double energy_drain = (moving ? 0.015 : 0.005) * dt + arousal * 0.008 * dt;
    if (b && b->asleep) {
        d->energy = fmin(1.0, d->energy + 0.08 * dt);
    } else {
        d->energy = fmax(0.0, d->energy - energy_drain);
    }

    /* B. Dinâmica de Estresse */
    if (arousal > 0.6) {
        d->stress = fmin(1.0, d->stress + 0.02 * dt);
    } else {
        d->stress = fmax(0.0, d->stress - 0.01 * dt);
    }

    /* C. Curiosidade */
    d->curiosity = fmin(1.0, fmax(0.0, d->curiosity + (0.01 - 0.005 * arousal) * dt));

    /* D. Saciedade (Fome) */
    if (b && b->eating) {
        d->satiety = fmin(1.0, d->satiety + 0.25 * dt);
    } else {
        d->satiety = fmax(0.0, d->satiety - 0.004 * dt);
    }

    /* E. Afeto */
    d->affection = fmin(1.0, fmax(0.0, d->affection + 0.001 * dt));
}

void homeostasis_event(Drives *d, const char *event_type) {
    if (!d || !event_type) return;

    if (!strcmp(event_type, "caress")) {
        d->stress = fmax(0.0, d->stress - 0.20);
        d->affection = fmin(1.0, d->affection + 0.15);
        d->curiosity = fmin(1.0, d->curiosity + 0.05);
    } else if (!strcmp(event_type, "poke")) {
        d->stress = fmin(1.0, d->stress + 0.25);
        d->affection = fmax(0.0, d->affection - 0.10);
        d->curiosity = fmin(1.0, d->curiosity + 0.10);
    } else if (!strcmp(event_type, "feed")) {
        d->satiety = fmin(1.0, d->satiety + 0.35);
        d->stress = fmax(0.0, d->stress - 0.10);
        d->energy = fmin(1.0, d->energy + 0.10);
    } else if (!strcmp(event_type, "talk")) {
        d->curiosity = fmin(1.0, d->curiosity + 0.15);
        d->affection = fmin(1.0, d->affection + 0.05);
    }
}
