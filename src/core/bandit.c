#include "bandit.h"
#include <math.h>
#include <string.h>

void bandit_init(DopaminergicBandit *b) {
    if (!b) return;
    memset(b, 0, sizeof(*b));
    b->exploration_c = 1.4142f; /* sqrt(2) padrão UCB1 */
    b->last_arm_chosen = -1;
}

BanditArm bandit_select_action(DopaminergicBandit *b, double now_s) {
    if (!b) return ARM_SILENT;

    /* 1. Exploração inicial: se algum braço nunca foi puxado, escolha-o */
    for (int i = 0; i < BANDIT_ARMS; ++i) {
        if (b->pulls[i] == 0) {
            b->last_arm_chosen = i;
            b->last_pull_time = now_s;
            return (BanditArm)i;
        }
    }

    /* 2. Cálculo do score UCB1: média + c * sqrt(2 * ln(N) / n_i) */
    float best_ucb = -1e9f;
    int best_arm = 0;
    float ln_total = logf((float)b->total_pulls);

    for (int i = 0; i < BANDIT_ARMS; ++i) {
        float mean_reward = b->rewards[i] / (float)b->pulls[i];
        float bonus = b->exploration_c * sqrtf((2.0f * ln_total) / (float)b->pulls[i]);
        float ucb = mean_reward + bonus;

        if (ucb > best_ucb) {
            best_ucb = ucb;
            best_arm = i;
        }
    }

    b->last_arm_chosen = best_arm;
    b->last_pull_time = now_s;
    return (BanditArm)best_arm;
}

void bandit_update_reward(DopaminergicBandit *b, BanditArm arm, float reward) {
    if (!b || arm < 0 || arm >= BANDIT_ARMS) return;

    /* Normaliza recompensa para [-1.0, 1.0] */
    if (reward > 1.0f) reward = 1.0f;
    if (reward < -1.0f) reward = -1.0f;

    b->pulls[arm]++;
    b->rewards[arm] += reward;
    b->total_pulls++;
}

size_t bandit_serialize(const DopaminergicBandit *b, uint8_t *out, size_t cap) {
    if (!b || !out) return 0;
    size_t needed = sizeof(uint32_t) * 2 + sizeof(b->pulls) + sizeof(b->rewards);
    if (cap < needed) return 0;

    uint32_t magic = 0x424E4454; /* "BNDT" */
    memcpy(out, &magic, 4);
    memcpy(out + 4, &b->total_pulls, 4);
    memcpy(out + 8, b->pulls, sizeof(b->pulls));
    memcpy(out + 8 + sizeof(b->pulls), b->rewards, sizeof(b->rewards));
    return needed;
}

bool bandit_deserialize(DopaminergicBandit *b, const uint8_t *in, size_t len) {
    if (!b || !in) return false;
    size_t needed = sizeof(uint32_t) * 2 + sizeof(b->pulls) + sizeof(b->rewards);
    if (len < needed) return false;

    uint32_t magic = 0;
    memcpy(&magic, in, 4);
    if (magic != 0x424E4454) return false;

    memcpy(&b->total_pulls, in + 4, 4);
    memcpy(b->pulls, in + 8, sizeof(b->pulls));
    memcpy(b->rewards, in + 8 + sizeof(b->pulls), sizeof(b->rewards));
    return true;
}
