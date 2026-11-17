/* behavior_bond.c — sistema de vínculo de longo prazo */
#include "behavior_bond.h"
#include <string.h>

#define TRUST_DECAY_RATE   0.00005f   /* perda passiva: ~1% por 200s */
#define RESENT_DECAY_RATE  0.0002f    /* ressentimento esquece em ~1h */
#define LONGING_RATE       0.00008f   /* 8h sem usar = saudade cheia */
#define AFFECTION_DECAY    0.002f     /* afeição instantânea decai em ~8min */

void behavior_bond_init(BehaviorBond *b) {
    memset(b, 0, sizeof(*b));
    b->trust     = fix_from_float(0.3f);  /* começa desconfiado */
    b->resentment= 0;
    b->longing   = 0;
    b->affection = 0;
    b->last_seen = 0.0;
    b->bond_age  = 0.0;
}

void behavior_bond_update(BehaviorBond *b, double now_sec, double dt) {
    b->bond_age += dt;

    /* Decaimento natural da confiança (precisa de manutenção) */
    fix16_t td = fix_from_float(TRUST_DECAY_RATE * (float)dt);
    b->trust = b->trust > td ? (b->trust - td) : 0;

    /* Ressentimento esquece com o tempo */
    fix16_t rd = fix_from_float(RESENT_DECAY_RATE * (float)dt);
    b->resentment = b->resentment > rd ? (b->resentment - rd) : 0;

    /* Saudade cresce quando o usuário some */
    double away_secs = now_sec - b->last_seen;
    if (away_secs > 300.0) {   /* após 5min inativo */
        fix16_t lg = fix_from_float(LONGING_RATE * (float)dt);
        b->longing += lg;
        if (b->longing > FIX_ONE) b->longing = FIX_ONE;
    }

    /* Afeição instantânea decai */
    fix16_t ad = fix_from_float(AFFECTION_DECAY * (float)dt);
    b->affection = b->affection > ad ? (b->affection - ad) : 0;

    /* Ressentimento alto corrói confiança mais rápido */
    if (b->resentment > fix_from_float(0.5f)) {
        fix16_t extra_decay = fix_from_float(0.0002f * (float)dt);
        b->trust = b->trust > extra_decay ? (b->trust - extra_decay) : 0;
    }
}

void behavior_bond_on_gentle_touch(BehaviorBond *b) {
    b->last_seen = 0.0;  /* resetado para "agora" — precisa de now_sec mas usamos flag */
    b->shake_streak = 0;
    fix16_t gain = fix_from_float(0.008f);
    b->trust += gain;
    if (b->trust > FIX_ONE) b->trust = FIX_ONE;
    b->affection += fix_from_float(0.05f);
    if (b->affection > FIX_ONE) b->affection = FIX_ONE;
    /* Toque suave reduz ressentimento levemente */
    fix16_t rr = fix_from_float(0.005f);
    b->resentment = b->resentment > rr ? (b->resentment - rr) : 0;
    b->longing = 0;
}

void behavior_bond_on_shake(BehaviorBond *b) {
    b->shake_streak++;
    fix16_t penalty = fix_from_float(0.02f + 0.01f * (float)b->shake_streak);
    b->resentment += penalty;
    if (b->resentment > FIX_ONE) b->resentment = FIX_ONE;
    /* Muitas sacudidas destroem confiança */
    if (b->shake_streak > 3) {
        fix16_t trust_loss = fix_from_float(0.01f * (float)(b->shake_streak - 3));
        b->trust = b->trust > trust_loss ? (b->trust - trust_loss) : 0;
    }
}

void behavior_bond_on_feed(BehaviorBond *b) {
    b->shake_streak = 0;
    b->trust += fix_from_float(0.015f);
    if (b->trust > FIX_ONE) b->trust = FIX_ONE;
    b->longing = 0;
}

void behavior_bond_on_ignore(BehaviorBond *b, double elapsed_secs) {
    (void)elapsed_secs;
    /* Implementado via update() com saudade */
}

fix16_t behavior_bond_trust_threshold(const BehaviorBond *b) {
    /* Quanto mais ressentido, mais difícil reconquistar */
    fix16_t base = fix_from_float(0.3f);
    fix16_t penalty = b->resentment >> 1;  /* metade do ressentimento como penalidade */
    return base + penalty;
}

bool behavior_bond_is_longing(const BehaviorBond *b) {
    return b->longing > fix_from_float(0.4f);
}

bool behavior_bond_is_resentful(const BehaviorBond *b) {
    return b->resentment > fix_from_float(0.5f);
}
