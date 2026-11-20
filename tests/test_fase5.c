#include "event_ring.h"
#include "bandit.h"
#include "sense_fatigue.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>

static void test_event_ring(void) {
    EventRingBuffer rb;
    event_ring_init(&rb);

    event_ring_push(&rb, EV_CODE_ERROR, 100, 10.0, 0.8f, "segfault");
    event_ring_push(&rb, EV_USER_TOUCH, 110, 12.0, 0.5f, "caress");

    assert(rb.count == 2);
    const TimedEvent *latest = event_ring_get(&rb, 0);
    assert(latest != NULL && latest->type == EV_USER_TOUCH);
    (void)latest;

    float intensity = 0;
    bool found = event_ring_has_recent(&rb, EV_CODE_ERROR, 15.0, 10.0, &intensity);
    assert(found == true);
    assert(fabsf(intensity - 0.8f) < 0.01f);

    /* Evento além da janela temporal (delta_s = 2s em t=15s) */
    found = event_ring_has_recent(&rb, EV_CODE_ERROR, 15.0, 2.0, &intensity);
    assert(found == false);
    (void)found;
    printf("  -> EventRingBuffer test: OK\n");
}

static void test_bandit(void) {
    DopaminergicBandit b;
    bandit_init(&b);

    /* 1. Primeiras 4 puxadas devem explorar todos os 4 braços */
    bool explored[BANDIT_ARMS] = {false};
    for (int i = 0; i < BANDIT_ARMS; ++i) {
        BanditArm arm = bandit_select_action(&b, 10.0 + i);
        explored[arm] = true;
        bandit_update_reward(&b, arm, 0.0f);
    }
    for (int i = 0; i < BANDIT_ARMS; ++i) {
        assert(explored[i] == true);
    }
    (void)explored;

    /* 2. Recompensa alta constante no braço ARM_SUGGEST_PAUSE (1) e negativa nos outros */
    for (int i = 0; i < 60; ++i) {
        BanditArm arm = bandit_select_action(&b, 20.0 + i);
        if (arm == ARM_SUGGEST_PAUSE) {
            bandit_update_reward(&b, arm, 1.0f);
        } else {
            bandit_update_reward(&b, arm, -0.5f);
        }
    }

    /* O braço mais puxado deve ser ARM_SUGGEST_PAUSE */
    assert(b.pulls[ARM_SUGGEST_PAUSE] > b.pulls[ARM_SILENT]);
    assert(b.pulls[ARM_SUGGEST_PAUSE] > b.pulls[ARM_CHEER_PLAY]);

    /* 3. Serialização / Desserialização */
    uint8_t buf[256];
    size_t sz = bandit_serialize(&b, buf, sizeof(buf));
    assert(sz > 0);

    DopaminergicBandit b2;
    bandit_init(&b2);
    bool ok = bandit_deserialize(&b2, buf, sz);
    assert(ok == true);
    (void)ok;
    assert(b2.total_pulls == b.total_pulls);
    assert(b2.pulls[ARM_SUGGEST_PAUSE] == b.pulls[ARM_SUGGEST_PAUSE]);

    printf("  -> DopaminergicBandit UCB1 test: OK\n");
}

static void test_fatigue(void) {
    SenseFatigue sf;
    sense_fatigue_init(&sf);

    assert(sense_fatigue_get_level(&sf) == 0.0f);

    /* Simula 1 hora de trabalho contínuo com digitação */
    for (int s = 0; s < 3600; ++s) {
        sense_fatigue_register_key(&sf, 'a', (double)s);
        sense_fatigue_update(&sf, 1.0, (double)s);
    }
    float f1 = sense_fatigue_get_level(&sf);
    assert(f1 > 0.25f);

    /* Simula pausa de 10 minutos (600 segundos ocioso) */
    for (int s = 3600; s < 4200; ++s) {
        sense_fatigue_update(&sf, 1.0, (double)s);
    }
    float f2 = sense_fatigue_get_level(&sf);
    assert(f2 < f1); /* Fadiga dissipou durante o descanso */
    (void)f1;
    (void)f2;

    printf("  -> SenseFatigue test: OK\n");
}

int main(void) {
    printf("[FASE5_TEST] Testando módulos da Fase 5...\n");
    test_event_ring();
    test_bandit();
    test_fatigue();
    printf("[FASE5_TEST] Todos os testes da Fase 5 passaram com sucesso!\n");
    return 0;
}
