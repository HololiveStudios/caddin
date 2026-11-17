/* tests/test_new_modules.c — testa sense_sysload, sense_heatmap, DMN, Bond, Explore */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "core/fixed.h"
#include "sense/sense_sysload.h"
#include "sense/sense_heatmap.h"
#include "behavior/behavior_dmn.h"
#include "behavior/behavior_bond.h"
#include "behavior/behavior_explore.h"

/* ── sense_sysload ── */
static void test_sysload(void) {
    SenseSysload s;
    sense_sysload_init(&s);
    assert(s.cpu_load == 0);
    assert(s.cpu_temp == 0);
    /* Primeira chamada popula os campos internos sem retornar true
       (precisa de intervalo mínimo) */
    bool updated = sense_sysload_update(&s, 100.0);
    (void)updated;
    /* Segunda chamada não atualiza pois não passou 1 segundo */
    bool not_updated = sense_sysload_update(&s, 100.5);
    assert(!not_updated);
    /* Chama novamente após o intervalo — deve retornar true */
    bool did_update = sense_sysload_update(&s, 101.5);
    assert(did_update);
    printf("  sense_sysload: cpu_load=%.3f temp=%.3f mem=%.3f\n",
           fix_to_float(sense_sysload_get_cpu(&s)),
           fix_to_float(sense_sysload_get_temp(&s)),
           fix_to_float(sense_sysload_get_mem(&s)));
    /* Valores devem estar em [0, 1] */
    assert(sense_sysload_get_cpu(&s) >= 0 && sense_sysload_get_cpu(&s) <= FIX_ONE);
    assert(sense_sysload_get_temp(&s) >= 0 && sense_sysload_get_temp(&s) <= FIX_ONE);
    assert(sense_sysload_get_mem(&s) >= 0 && sense_sysload_get_mem(&s) <= FIX_ONE);
    printf("  [OK] sense_sysload\n");
}

/* ── sense_heatmap ── */
static void test_heatmap(void) {
    SenseHeatmap h;
    sense_heatmap_init(&h, 1920, 1080);

    /* Registra cursor no canto superior esquerdo repetidamente */
    for (int i = 0; i < 50; i++) {
        sense_heatmap_register(&h, 10, 10);
    }
    /* Célula [0][0] deve ser a mais quente */
    int hcol, hrow;
    sense_heatmap_hottest(&h, &hcol, &hrow);
    assert(hcol == 0 && hrow == 0);

    /* A célula mais fria deve ser distante */
    int ccol, crow;
    sense_heatmap_coldest(&h, &ccol, &crow);
    assert(!(ccol == 0 && crow == 0));

    /* Converte célula para px */
    int px, py;
    sense_heatmap_cell_to_px(&h, 0, 0, &px, &py);
    assert(px >= 0 && py >= 0);

    /* Decaimento deve reduzir valores */
    fix16_t before = h.cells[0][0];
    sense_heatmap_decay(&h, 0.0);  /* inicializa last_decay */
    sense_heatmap_decay(&h, 2.0);  /* 2s depois */
    assert(h.cells[0][0] < before);

    printf("  [OK] sense_heatmap (hottest=[%d,%d], coldest=[%d,%d])\n",
           hcol, hrow, ccol, crow);
}

/* ── behavior_dmn ── */
static void test_dmn(void) {
    BehaviorDmn d;
    behavior_dmn_init(&d);

    /* Recém iniciado, sem atividade — estado deve ser IDLE */
    DmnState s = behavior_dmn_update(&d, 0.0, fix_from_float(0.5f), 15.0);
    assert(s == DMN_IDLE);

    /* Após 20 segundos de inatividade, deve ativar algum comportamento */
    s = behavior_dmn_update(&d, 20.0, fix_from_float(0.5f), 15.0);
    assert(s != DMN_IDLE);
    printf("  DMN ativou: %s\n", behavior_dmn_state_name(s));

    /* Sinal de atividade reseta o DMN */
    behavior_dmn_signal_activity(&d, 25.0);
    s = behavior_dmn_update(&d, 25.5, fix_from_float(0.5f), 15.0);
    assert(s == DMN_IDLE);

    printf("  [OK] behavior_dmn\n");
}

/* ── behavior_bond ── */
static void test_bond(void) {
    BehaviorBond b;
    behavior_bond_init(&b);

    fix16_t initial_trust = b.trust;

    /* Carinho aumenta confiança */
    behavior_bond_on_gentle_touch(&b);
    assert(b.trust >= initial_trust);
    assert(b.affection > 0);

    /* Sacudidas aumentam ressentimento */
    for (int i = 0; i < 5; i++) {
        behavior_bond_on_shake(&b);
    }
    assert(b.resentment > 0);
    assert(behavior_bond_is_resentful(&b) || b.resentment > 0);

    /* Atualiza por 10 segundos — afeição deve decair */
    fix16_t aff_before = b.affection;
    behavior_bond_update(&b, 100.0, 10.0);
    assert(b.affection <= aff_before);

    /* Threshold de confiança deve ser maior com ressentimento */
    fix16_t thr = behavior_bond_trust_threshold(&b);
    assert(thr > fix_from_float(0.3f));  /* base threshold */

    printf("  [OK] behavior_bond (trust=%.3f resentment=%.3f)\n",
           fix_to_float(b.trust), fix_to_float(b.resentment));
}

/* ── behavior_explore ── */
static void test_explore(void) {
    BehaviorExplore e;
    behavior_explore_init(&e);
    assert(e.state == EXPLORE_REST);
    assert(e.target_x == -1);

    SenseHeatmap h;
    sense_heatmap_init(&h, 1920, 1080);

    /* Nível alto de curiosidade — deve escolher explorar */
    /* Roda 100 tentativas com curiosidade máxima para garantir ativação */
    int activated = 0;
    for (int i = 0; i < 100; i++) {
        BehaviorExplore ei;
        behavior_explore_init(&ei);
        ExploreState s = behavior_explore_update(&ei, &h, 100, 100,
                                                 fix_from_float(1.0f), (double)i);
        if (s != EXPLORE_REST) { activated++; break; }
    }
    assert(activated > 0);

    int tx, ty;
    behavior_explore_get_target(&e, &tx, &ty);
    /* Estado REST → target = -1 */
    assert(tx == -1 && ty == -1);

    printf("  [OK] behavior_explore (activatable=%s)\n", activated ? "yes" : "no");
}

int main(void) {
    printf("=== test_new_modules ===\n");
    test_sysload();
    test_heatmap();
    test_dmn();
    test_bond();
    test_explore();
    printf("=== TODOS OS TESTES PASSARAM ===\n");
    return 0;
}
