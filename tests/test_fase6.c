#include "sense_keystroke_biometrics.h"
#include "daily_summary.h"
#include "telemetry_fifo.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>

static void test_keystroke_biometrics(void) {
    SenseKeystrokeBiometrics kb;
    sense_keystroke_init(&kb);

    assert(kb.total_strokes == 0);
    assert(kb.owner_learned == false);

    /* 1. Simula digitação cadenciada do dono (intervalos em torno de 150ms) */
    double t = 10.0;
    for (int i = 0; i < 120; ++i) {
        sense_keystroke_press(&kb, 'a', t);
        t += 0.150; /* 150 ms */
    }
    assert(kb.current_samples >= 100);

    /* 2. Consolidação de Imprinting */
    bool consolidated = sense_keystroke_consolidate_imprint(&kb);
    assert(consolidated == true);
    (void)consolidated;
    assert(kb.owner_learned == true);

    /* 3. Simula mais digitação do mesmo dono (150ms) -> similaridade deve ser alta (> 0.90) */
    for (int i = 0; i < 40; ++i) {
        sense_keystroke_press(&kb, 'x', t);
        t += 0.150;
    }
    float sim_owner = sense_keystroke_get_owner_similarity(&kb);
    assert(sim_owner > 0.85f);
    (void)sim_owner;

    /* 4. Simula digitação de um estranho (muito lenta e irregular, ~800ms) */
    SenseKeystrokeBiometrics kb_stranger = kb;
    memset(kb_stranger.current_histogram, 0, sizeof(kb_stranger.current_histogram));
    kb_stranger.current_samples = 0;
    for (int i = 0; i < 50; ++i) {
        sense_keystroke_press(&kb_stranger, 'y', t);
        t += 0.800;
    }
    float sim_stranger = sense_keystroke_get_owner_similarity(&kb_stranger);
    assert(sim_stranger < sim_owner); /* Ritmo diferente tem menor similaridade */
    (void)sim_stranger;

    /* 5. Serialização / Desserialização */
    uint8_t buf[256];
    size_t sz = sense_keystroke_serialize(&kb, buf, sizeof(buf));
    assert(sz > 0);

    SenseKeystrokeBiometrics kb2;
    sense_keystroke_init(&kb2);
    bool ok = sense_keystroke_deserialize(&kb2, buf, sz);
    assert(ok == true);
    (void)ok;
    assert(kb2.owner_learned == true);
    assert(kb2.owner_samples == kb.owner_samples);

    printf("  -> KeystrokeBiometrics test: OK\n");
}

static void test_daily_summary(void) {
    DailySummary ds;
    daily_summary_init(&ds, 100);

    assert(ds.current_day == 100);
    assert(ds.active_seconds == 0);

    /* Simula 60 segundos acordado com eventos */
    for (int i = 0; i < 60; ++i) {
        daily_summary_tick_1hz(&ds, 100, false, 0.8f);
    }
    daily_summary_record_key(&ds);
    daily_summary_record_key(&ds);
    daily_summary_record_touch(&ds);
    daily_summary_record_code_success(&ds);
    daily_summary_record_code_error(&ds);
    daily_summary_record_intervention(&ds);

    assert(ds.active_seconds == 60);
    assert(ds.typing_keys_count == 2);
    assert(ds.touches_count == 1);
    assert(ds.code_success_count == 1);
    assert(ds.code_errors_count == 1);
    assert(ds.fatigue_interventions_count == 1);

    /* Formatação de texto */
    char buf[1024];
    size_t len = daily_summary_format(&ds, buf, sizeof(buf));
    assert(len > 0);
    (void)len;
    assert(strstr(buf, "RESUMO DO DIA") != NULL);
    assert(strstr(buf, "00h 01m 00s") != NULL);

    /* Teste de mudança de dia (reseta contadores diários) */
    daily_summary_tick_1hz(&ds, 101, false, 0.5f);
    assert(ds.current_day == 101);
    assert(ds.active_seconds == 1);
    assert(ds.typing_keys_count == 0);

    printf("  -> DailySummary test: OK\n");
}

static void test_telemetry_frame(void) {
    CaddinTelemetryFrame frame;
    memset(&frame, 0, sizeof(frame));
    frame.magic = TELEMETRY_MAGIC;
    frame.tick = 12345;
    frame.da = 0.8f;
    frame.onto_stage = 2;

    assert(frame.magic == 0x43414454);
    assert(sizeof(frame.pool_activity) == 120);
    printf("  -> TelemetryFrame structure test: OK (size: %zu bytes)\n", sizeof(frame));
}

int main(void) {
    printf("[FASE6_TEST] Testando módulos da Fase 6 (100%% C Puro)...\n");
    test_keystroke_biometrics();
    test_daily_summary();
    test_telemetry_frame();
    printf("[FASE6_TEST] Todos os testes da Fase 6 passaram com sucesso!\n");
    return 0;
}
