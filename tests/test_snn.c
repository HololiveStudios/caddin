#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "../src/core/fixed.h"
#include "../src/core/arena.h"
#include "../src/core/snn.h"

static void test_snn_basic_dynamics(void) {
    printf("[TEST] Dinâmica LIF e Disparo Sub/Super-limiar...\n");
    Arena *arena = arena_create(1024 * 512); /* 512 KB */
    assert(arena != NULL);

    SnnNetwork snn;
    bool ok = snn_init(&snn, arena, 10, 20, 3);
    assert(ok);

    /* Injetar corrente fraca: não deve disparar */
    snn_inject_current(&snn, 0, fix_from_float(0.4f));
    snn_step(&snn);
    assert(!snn_is_spiking(&snn, 0));
    assert(snn_get_voltage(&snn, 0) > 0);

    /* Injetar corrente forte: deve disparar */
    snn_inject_current(&snn, 0, fix_from_float(1.5f));
    snn_step(&snn);
    assert(snn_is_spiking(&snn, 0));
    assert(snn_get_voltage(&snn, 0) == SNN_RESET_V);

    /* Próximo tick: período refratário deve impedir disparo mesmo com corrente */
    snn_inject_current(&snn, 0, fix_from_float(2.0f));
    snn_step(&snn);
    assert(!snn_is_spiking(&snn, 0));

    arena_destroy(arena);
    printf("  -> Dinâmica LIF: OK\n");
}

static void test_snn_synaptic_delay_and_stp(void) {
    printf("[TEST] Transmissão CSR com Delay e Fadiga STP...\n");
    Arena *arena = arena_create(1024 * 512);
    SnnNetwork snn;
    snn_init(&snn, arena, 5, 10, 3);

    /* Conectar neurônio 0 -> neurônio 1 com peso forte e delay de 2 ticks */
    snn_add_synapse(&snn, 0, 1, fix_from_float(2.0f), 2);

    /* Forçar spike no neurônio 0 */
    snn_inject_current(&snn, 0, fix_from_float(2.0f));
    snn_step(&snn);
    assert(snn_is_spiking(&snn, 0));
    assert(!snn_is_spiking(&snn, 1)); /* Ainda não chegou */

    /* Tick 1 após spike: corrente em trânsito no buffer de retardo */
    snn_step(&snn);
    assert(!snn_is_spiking(&snn, 1));

    /* Tick 2 após spike: corrente atinge o neurônio 1 e o faz disparar! */
    snn_step(&snn);
    assert(snn_is_spiking(&snn, 1));

    /* Teste de fadiga STP: sinapse sofreu consumo de neurotransmissor */
    assert(snn.stp_x[0] < 255);

    arena_destroy(arena);
    printf("  -> Delay e STP: OK\n");
}

static void test_snn_r_stdp(void) {
    printf("[TEST] Plasticidade R-STDP Trifatorial (Dopamina)...\n");
    Arena *arena = arena_create(1024 * 512);
    SnnNetwork snn;
    snn_init(&snn, arena, 4, 10, 3);

    snn_add_synapse(&snn, 0, 1, fix_from_float(0.5f), 1);
    fix16_t w_initial = snn.w[0];

    /* Simular elegibilidade artificial */
    snn.elig[0] = fix_from_float(0.8f);

    /* Sem dopamina (delta = 0), peso não deve mudar */
    snn_apply_plasticity(&snn, fix_from_float(0.0f), fix_from_float(0.1f));
    assert(snn.w[0] == w_initial);

    /* Com dopamina positiva (recompensa), peso deve aumentar */
    snn_apply_plasticity(&snn, fix_from_float(1.0f), fix_from_float(0.2f));
    assert(snn.w[0] > w_initial);

    arena_destroy(arena);
    printf("  -> R-STDP Trifatorial: OK\n");
}

static void test_snn_benchmark_realtime(void) {
    printf("[TEST] Benchmark de Desempenho e Zero Mallocs (2048 Neurônios)...\n");
    Arena *arena = arena_create(1024 * 1024 * 4); /* 4 MB */
    SnnNetwork snn;
    uint32_t n_neurons = 2048;
    uint32_t n_synapses = 2048 * 32; /* ~65k sinapses */
    snn_init(&snn, arena, n_neurons, n_synapses, 3);

    /* Conectar em topologia esparsa */
    for (uint32_t i = 0; i < n_neurons; ++i) {
        for (uint32_t j = 0; j < 32; ++j) {
            uint32_t target = (i + j + 1) % n_neurons;
            snn_add_synapse(&snn, i, target, fix_from_float(0.1f), (uint8_t)(j % 4));
        }
    }

    size_t memory_used = arena_used(arena);
    printf("  -> Memória utilizada para 2048 neurônios e 65k sinapses: %.2f MB\n", (double)memory_used / (1024.0 * 1024.0));
    assert(memory_used < 3 * 1024 * 1024); /* Menos de 3 MB! */

    clock_t start = clock();
    for (int t = 0; t < 500; ++t) {
        /* Estímulo em alguns neurônios */
        snn_inject_current(&snn, t % n_neurons, fix_from_float(1.2f));
        snn_step(&snn);
        /* Garantir zero crescimento de memória durante o laço */
        assert(arena_used(arena) == memory_used);
    }
    clock_t end = clock();
    double elapsed_sec = (double)(end - start) / CLOCKS_PER_SEC;
    printf("  -> 500 ticks executados em %.4f s (%.1f ticks/s)\n", elapsed_sec, 500.0 / elapsed_sec);

    arena_destroy(arena);
    printf("  -> Benchmark & Zero Mallocs: OK\n");
}

int main(void) {
    printf("=== EXECUÇÃO LOTE 1.1 & 1.2: MOTOR SNN AVANÇADO + R-STDP + STP ===\n");
    test_snn_basic_dynamics();
    test_snn_synaptic_delay_and_stp();
    test_snn_r_stdp();
    test_snn_benchmark_realtime();
    printf("=== LOTE 1.1 & 1.2 APROVADO COM SUCESSO ===\n");
    return 0;
}
