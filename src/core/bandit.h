#ifndef PET_CORE_BANDIT_H
#define PET_CORE_BANDIT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define BANDIT_ARMS 4

typedef enum {
    ARM_SILENT = 0,       /* Ficar discreto e silencioso */
    ARM_SUGGEST_PAUSE,    /* Sugerir pausa / descanso */
    ARM_CHEER_PLAY,       /* Brincadeira / trilo motivacional */
    ARM_STRETCH_WATER     /* Alongamento / lembrete de postura */
} BanditArm;

typedef struct {
    uint32_t pulls[BANDIT_ARMS];
    float    rewards[BANDIT_ARMS];
    uint32_t total_pulls;
    int      last_arm_chosen;
    double   last_pull_time;
    float    exploration_c; /* Coeficiente UCB (ex: 1.414f) */
} DopaminergicBandit;

void      bandit_init(DopaminergicBandit *b);
BanditArm bandit_select_action(DopaminergicBandit *b, double now_s);
void      bandit_update_reward(DopaminergicBandit *b, BanditArm arm, float reward);

size_t    bandit_serialize(const DopaminergicBandit *b, uint8_t *out, size_t cap);
bool      bandit_deserialize(DopaminergicBandit *b, const uint8_t *in, size_t len);

#endif /* PET_CORE_BANDIT_H */
