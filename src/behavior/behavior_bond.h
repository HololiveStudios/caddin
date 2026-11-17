/* behavior_bond.h — sistema de vínculo e afeição de longo prazo
 * Modela confiança acumulada, ressentimento por maus-tratos e
 * o "desejo de companhia" (longing) quando o usuário some por muito tempo.
 */
#ifndef BEHAVIOR_BOND_H
#define BEHAVIOR_BOND_H

#include <stdbool.h>
#include "../core/fixed.h"

typedef struct {
    fix16_t trust;         /* 0..1 — confiança acumulada */
    fix16_t resentment;    /* 0..1 — ressentimento por sacudidas/maus-tratos */
    fix16_t longing;       /* 0..1 — saudade (cresce quando usuário some) */
    fix16_t affection;     /* 0..1 — afeição instantânea (decai rápido) */

    int     shake_streak;  /* contador de sacudidas consecutivas recentes */
    double  last_seen;     /* última vez que houve interação */
    double  bond_age;      /* tempo total de vida do vínculo (segundos) */
} BehaviorBond;

void    behavior_bond_init(BehaviorBond *b);
void    behavior_bond_update(BehaviorBond *b, double now_sec, double dt);

/* Registra eventos positivos e negativos */
void    behavior_bond_on_gentle_touch(BehaviorBond *b);
void    behavior_bond_on_shake(BehaviorBond *b);
void    behavior_bond_on_feed(BehaviorBond *b);
void    behavior_bond_on_ignore(BehaviorBond *b, double elapsed_secs);

/* Retorna limiar atual de confiança (maior resentment = limiar mais alto) */
fix16_t behavior_bond_trust_threshold(const BehaviorBond *b);
bool    behavior_bond_is_longing(const BehaviorBond *b);
bool    behavior_bond_is_resentful(const BehaviorBond *b);

#endif /* BEHAVIOR_BOND_H */
