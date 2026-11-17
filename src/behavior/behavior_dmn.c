/* behavior_dmn.c — Default Mode Network */
#define _POSIX_C_SOURCE 200809L
#include "behavior_dmn.h"
#include <string.h>
#include <stdlib.h>

static const char *DMN_NAMES[] = {
    "IDLE", "DAYDREAM", "GROOMING", "YAWN", "LOOK_AROUND", "MICRO_CHIRP"
};

void behavior_dmn_init(BehaviorDmn *d) {
    memset(d, 0, sizeof(*d));
    d->state    = DMN_IDLE;
    d->arousal  = fix_from_float(0.5f);
}

void behavior_dmn_signal_activity(BehaviorDmn *d, double now_sec) {
    d->idle_since = now_sec;
    if (d->state != DMN_IDLE) {
        d->state = DMN_IDLE;
    }
}

static DmnState pick_dmn_state(fix16_t arousal) {
    /* Quanto menor o arousal, mais sonolento o comportamento */
    float ar = fix_to_float(arousal);
    int r = rand() % 100;
    if (ar < 0.25f) {
        /* Muito cansado */
        if (r < 50) return DMN_YAWN;
        if (r < 80) return DMN_DAYDREAM;
        return DMN_GROOMING;
    } else if (ar < 0.60f) {
        /* Moderado */
        if (r < 30) return DMN_LOOK_AROUND;
        if (r < 55) return DMN_GROOMING;
        if (r < 70) return DMN_DAYDREAM;
        if (r < 85) return DMN_MICRO_CHIRP;
        return DMN_YAWN;
    } else {
        /* Alerta */
        if (r < 40) return DMN_LOOK_AROUND;
        if (r < 65) return DMN_MICRO_CHIRP;
        if (r < 85) return DMN_GROOMING;
        return DMN_DAYDREAM;
    }
}

static double dmn_duration(DmnState s, fix16_t arousal) {
    (void)arousal;
    switch (s) {
        case DMN_YAWN:        return 1.5;
        case DMN_DAYDREAM:    return 4.0 + (rand() % 60) * 0.1;
        case DMN_GROOMING:    return 3.0 + (rand() % 40) * 0.1;
        case DMN_LOOK_AROUND: return 2.0;
        case DMN_MICRO_CHIRP: return 0.8;
        default:              return 2.0;
    }
}

DmnState behavior_dmn_update(BehaviorDmn *d, double now_sec,
                              fix16_t arousal, double idle_threshold) {
    d->arousal = arousal;
    double idle_secs = now_sec - d->idle_since;

    if (d->state != DMN_IDLE) {
        /* Verifica se o estado atual terminou */
        if (now_sec - d->state_start >= d->state_duration) {
            d->state = DMN_IDLE;
        }
        return d->state;
    }

    /* Ativa DMN após idle_threshold segundos de inatividade */
    if (idle_secs < idle_threshold) return DMN_IDLE;

    /* Escolhe um novo comportamento espontâneo */
    d->state          = pick_dmn_state(arousal);
    d->state_start    = now_sec;
    d->state_duration = dmn_duration(d->state, arousal);
    return d->state;
}

bool behavior_dmn_wants_attention(const BehaviorDmn *d) {
    return (d->state == DMN_MICRO_CHIRP || d->state == DMN_LOOK_AROUND);
}

const char *behavior_dmn_state_name(DmnState s) {
    if (s < 0 || s >= DMN_STATE_COUNT) return "???";
    return DMN_NAMES[s];
}
