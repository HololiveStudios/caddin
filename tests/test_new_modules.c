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
#include "behavior/behavior_hide_hunt.h"
#include "behavior/behavior_perch.h"
#include "behavior/behavior_startle_relief.h"
#include "sense/sense_mic.h"
#include "sense/sense_text.h"

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

/* ── behavior_hide_hunt ── */
static void test_hide_hunt(void) {
    BehaviorHideHunt hh;
    behavior_hide_hunt_init(&hh);
    assert(hh.state == HIDE_HUNT_IDLE);

    float dx = 0, dy = 0;
    /* Com estresse alto (> 0.65), deve entrar em HIDE_HUNT_HIDING */
    behavior_hide_hunt_update(&hh, 0.1f, 500, 300, 600, 300, 200, 100, 800, 600,
                              0.85f, 0.5f, 0.5f, &dx, &dy);
    assert(hh.state == HIDE_HUNT_HIDING);
    assert(dx != 0.0f); /* Moveu em direção à borda */

    /* Com baixo estresse e alta energia/curiosidade, stalkeia o cursor */
    behavior_hide_hunt_init(&hh);
    behavior_hide_hunt_update(&hh, 0.1f, 500, 300, 700, 300, 200, 100, 800, 600,
                              0.10f, 0.80f, 0.70f, &dx, &dy);
    assert(hh.state == HIDE_HUNT_STALKING);
    printf("  [OK] behavior_hide_hunt\n");
}

/* ── behavior_perch ── */
static void test_perch(void) {
    BehaviorPerch p;
    behavior_perch_init(&p);
    assert(p.state == PERCH_STATE_FREE);

    float dx = 0, dy = 0;
    /* Próximo ao topo da janela (win_y = 300, target_y = 190, pet_y = 200) e calmo */
    bool perched = behavior_perch_update(&p, 0.1f, 400, 200, 200, 300, 600, 400,
                                         0.4f, 0.1f, &dx, &dy);
    (void)perched;
    assert(p.state == PERCH_STATE_APPROACHING);

    /* Vários passos para concluir o alinhamento */
    for (int i = 0; i < 20; ++i) {
        behavior_perch_update(&p, 0.1f, 400, 190, 200, 300, 600, 400,
                              0.4f, 0.1f, &dx, &dy);
    }
    assert(p.state == PERCH_STATE_PERCHED);
    assert(p.is_perched);
    printf("  [OK] behavior_perch\n");
}

/* ── behavior_startle_relief ── */
static void test_startle_relief(void) {
    BehaviorStartleRelief sr;
    behavior_startle_relief_init(&sr);
    assert(sr.phase == STARTLE_PHASE_CALM);

    behavior_startle_relief_trigger(&sr, 0.9f);
    assert(sr.phase == STARTLE_PHASE_SHOCK);

    float squish = 0, ser_boost = 0;
    bool relieved = false;
    /* Executa durante 1.3s para disparar a transição para a Fase 2 (Alívio) */
    for (int i = 0; i < 15; ++i) {
        if (behavior_startle_relief_update(&sr, 0.1f, &squish, &ser_boost)) {
            relieved = true;
        }
    }
    assert(relieved);
    assert(sr.phase == STARTLE_PHASE_RELIEF);
    assert(ser_boost > 0.0f); /* Injeção de serotonina de alívio */

    printf("  [OK] behavior_startle_relief (bifásico susto->alívio)\n");
}

/* ── sense_mic & sense_text ── */
static void test_mic_and_text(void) {
    SenseMic mic;
    sense_mic_init(&mic, 16000);
    /* Update gracioso mesmo sem microfone físico */
    sense_mic_update(&mic, 0.016f);
    float rms = sense_mic_get_rms(&mic);
    assert(rms >= 0.0f && rms <= 1.0f);
    sense_mic_destroy(&mic);

    SenseText st;
    sense_text_init(&st);
    sense_text_update(&st, 0.6f, NULL, 0);
    const char *w = sense_text_get_last_word(&st);
    assert(w != NULL);

    printf("  [OK] sense_mic e sense_text\n");
}

int main(void) {
    printf("=== test_new_modules ===\n");
    test_sysload();
    test_heatmap();
    test_dmn();
    test_bond();
    test_explore();
    test_hide_hunt();
    test_perch();
    test_startle_relief();
    test_mic_and_text();
    printf("=== TODOS OS TESTES PASSARAM ===\n");
    return 0;
}
