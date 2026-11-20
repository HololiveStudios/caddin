#include "rituals.h"
#include <stdio.h>
#include <assert.h>

static uint32_t triggered_milestone = 0;
static int milestone_calls = 0;

static void on_milestone(uint32_t hours, void *ctx) {
    (void)ctx;
    triggered_milestone = hours;
    milestone_calls++;
}

static int morning_count = 0;
static int night_count = 0;

static void on_circadian(bool is_morning, void *ctx) {
    (void)ctx;
    if (is_morning) morning_count++;
    else night_count++;
}

int main(void) {
    RitualState rs;
    rituals_init(&rs);

    /* 1. Teste de marcos de convivência */
    /* Menos de 1h (1800s): nenhum marco */
    rituals_check_milestones(&rs, 1800, on_milestone, NULL);
    assert(milestone_calls == 0);

    /* 1 hora atingida (3600s): marco de 1h */
    rituals_check_milestones(&rs, 3600, on_milestone, NULL);
    assert(milestone_calls == 1);
    assert(triggered_milestone == 1);

    /* Chamada subsequente com mesmo tempo: não deve disparar novamente */
    rituals_check_milestones(&rs, 3650, on_milestone, NULL);
    assert(milestone_calls == 1);

    /* 10 horas atingidas (36000s): marco de 10h */
    rituals_check_milestones(&rs, 36000, on_milestone, NULL);
    assert(milestone_calls == 2);
    assert(triggered_milestone == 5 || triggered_milestone == 10);

    /* 2. Teste de saudações circadianas */
    /* Dia 1, 9h da manhã */
    rituals_check_circadian(&rs, 9, 1, on_circadian, NULL);
    assert(morning_count == 1);

    /* Mesma manhã: não repete */
    rituals_check_circadian(&rs, 10, 1, on_circadian, NULL);
    assert(morning_count == 1);

    /* Noite (23h): boa noite */
    rituals_check_circadian(&rs, 23, 1, on_circadian, NULL);
    assert(night_count == 1);

    /* Próximo dia (Dia 2): manhã deve acionar novamente */
    rituals_check_circadian(&rs, 8, 2, on_circadian, NULL);
    assert(morning_count == 2);

    printf("[RITUALS_TEST] Todos os testes passaram com sucesso!\n");
    return 0;
}
