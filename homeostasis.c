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

    /* A. Decaimento passivo de energia */
    double energy_drain = (moving ? 0.015 : 0.005) * dt + arousal * 0.008 * dt;
    if (b && b->asleep) {
        d->energy = fmin(1.0, d->energy + 0.08 * dt);
    } else {
        d->energy = fmax(0.0, d->energy - energy_drain);
    }

    /* B. Dinâmica de Estresse — agora modulada pelos sensores neurais */
    if (arousal > 0.6) {
        d->stress = fmin(1.0, d->stress + 0.02 * dt);
    } else {
        d->stress = fmax(0.0, d->stress - 0.01 * dt);
    }

    /* Estímulos sensoriais diretos afetam drives (via canais neurais) */
    if (inputs) {
        /* Canal 16 (CARESS) reduz estresse e aumenta afeição */
        if (inputs[16] > 0.3) {
            d->stress    = fmax(0.0, d->stress    - inputs[16] * 0.08 * dt);
            d->affection = fmin(1.0, d->affection + inputs[16] * 0.06 * dt);
        }
        /* Canal 17 (POKE) aumenta estresse */
        if (inputs[17] > 0.3) {
            d->stress    = fmin(1.0, d->stress    + inputs[17] * 0.10 * dt);
            d->affection = fmax(0.0, d->affection - inputs[17] * 0.03 * dt);
        }
        /* Canal 18 (FEED) aumenta saciedade */
        if (inputs[18] > 0.3) {
            d->satiety = fmin(1.0, d->satiety + inputs[18] * 0.12 * dt);
            d->energy  = fmin(1.0, d->energy  + inputs[18] * 0.05 * dt);
        }
        /* Canal 31 (NOISE / sacudida) gera estresse */
        if (inputs[31] > 0.3) {
            d->stress = fmin(1.0, d->stress + inputs[31] * 0.12 * dt);
        }
        /* Canal 42 (USER_PRESENT) reduz solidão */
        if (inputs[42] > 0.2) {
            d->affection = fmin(1.0, d->affection + inputs[42] * 0.02 * dt);
        }
        /* Canal 43 (USER_AWAY) gera tédio */
        if (inputs[43] > 0.3) {
            d->curiosity = fmax(0.0, d->curiosity - inputs[43] * 0.01 * dt);
        }
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

/* ── Calcula fator de recompensa endógeno a partir dos drives ── */
double homeostasis_compute_reward(const Drives *d) {
    if (!d) return 0.0;

    double reward = 0.0;

    /* Recompensa positiva: alívio de estresse, saciedade, afeto */
    if (d->stress < 0.3) reward += 0.2;             /* Calmo → recompensa leve */
    if (d->affection > 0.6) reward += 0.15;          /* Afeto alto → dopamina */
    if (d->satiety > 0.5) reward += 0.10;            /* Alimentado → bem-estar */
    if (d->energy > 0.5 && d->stress < 0.4) reward += 0.10; /* Energia boa e relaxado */

    /* Punição endógena: estresse alto, fome, exaustão */
    if (d->stress > 0.7) reward -= 0.3;              /* Estressado → cortisol */
    if (d->satiety < 0.2) reward -= 0.15;            /* Faminto → desconforto */
    if (d->energy < 0.15) reward -= 0.20;            /* Exausto → punição */

    /* Limitar entre -1 e +1 */
    if (reward > 1.0) reward = 1.0;
    if (reward < -1.0) reward = -1.0;

    return reward;
}
