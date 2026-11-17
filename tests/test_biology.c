#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "../src/core/fixed.h"
#include "../src/core/neuromod.h"
#include "../src/core/cpg.h"
#include "../src/core/predict.h"

static void test_neuromod_and_nociception(void) {
    printf("[TEST] Neuromodulação e Reflexo Nociceptivo...\n");
    Neuromodulators nm;
    neuromod_init(&nm);

    assert(nm.da == nm.base_da);
    assert(nm.ne == nm.base_ne);

    /* Injeção de dopamina e decaimento gradual */
    neuromod_inject_da(&nm, fix_from_float(0.8f));
    assert(nm.da > nm.base_da);
    for (int i = 0; i < 50; ++i) neuromod_step(&nm);
    assert(nm.da < fix_from_float(1.2f)); /* Relaxou em direção à base */

    /* Ganho sensorial: com NE baixa ganho é próximo de 1.0; com NE alta o ganho sobe */
    fix16_t gain_low = neuromod_get_sensory_gain(&nm);
    neuromod_inject_ne(&nm, fix_from_float(1.0f));
    fix16_t gain_high = neuromod_get_sensory_gain(&nm);
    assert(gain_high > gain_low);

    /* Teste do reflexo nociceptivo */
    fix16_t jolt = 0;
    bool triggered = neuromod_check_nociceptive_reflex(&nm, fix_from_float(0.5f), &jolt);
    assert(!triggered); /* Abaixo do limiar */

    triggered = neuromod_check_nociceptive_reflex(&nm, fix_from_float(2.0f), &jolt);
    assert(triggered);  /* Acima do limiar: reflexo instantâneo! */
    assert(jolt > 0);
    assert(nm.cort > nm.base_cort);

    printf("  -> Neuromodulação e Nocicepção: OK\n");
}

static void test_cpg_and_blackout(void) {
    printf("[TEST] CPG (Respiração/Pulso) e Febre Cognitiva...\n");
    CentralPatternGenerator cpg;
    cpg_init(&cpg);

    /* Respiração deve oscilar */
    fix16_t b1 = cpg_get_breath_expansion(&cpg);
    for (int i = 0; i < 20; ++i) {
        cpg_step(&cpg, 0, 5, 100);
    }
    fix16_t b2 = cpg_get_breath_expansion(&cpg);
    assert(b1 != b2); /* Fase mudou */

    /* Teste de febre cognitiva: simular 100% de neurônios disparando */
    assert(!cpg_is_in_blackout(&cpg));
    for (int i = 0; i < 60; ++i) {
        cpg_step(&cpg, 0, 100, 100); /* 100 disparos em 100 neurônios! */
    }
    assert(cpg_is_in_blackout(&cpg)); /* Blackout acionado com sucesso */

    printf("  -> CPG e Febre Cognitiva: OK\n");
}

static void test_predictive_coding(void) {
    printf("[TEST] Codificação Preditiva e Mismatch Spikes...\n");
    TemporalPredictor pred;
    predict_init(&pred);

    fix16_t reward = 0;
    /* Eventos regulares a cada 50 ticks */
    predict_register_event(&pred, 100, &reward);
    predict_register_event(&pred, 150, &reward);
    predict_register_event(&pred, 200, &reward);
    assert(reward > 0); /* Confirmação preditiva gerou dopamina */

    /* Avançar tempo até passar da janela esperada (250 + tolerância) */
    fix16_t surprise = 0;
    bool mismatch = false;
    for (uint32_t t = 201; t <= 270; ++t) {
        if (predict_step(&pred, t, &surprise)) {
            mismatch = true;
            break;
        }
    }
    assert(mismatch);      /* Disparou Mismatch Spike por ausência do evento! */
    assert(surprise > 0);  /* Gerou Noradrenalina de surpresa */

    printf("  -> Codificação Preditiva: OK\n");
}

int main(void) {
    printf("=== EXECUÇÃO FASE 2: BIOLOGIA, CPG, NEUROMODULAÇÃO E PREDIÇÃO ===\n");
    test_neuromod_and_nociception();
    test_cpg_and_blackout();
    test_predictive_coding();
    printf("=== FASE 2 APROVADA COM SUCESSO ===\n");
    return 0;
}
