#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "../src/core/fixed.h"
#include "../src/core/arena.h"
#include "../src/core/rng.h"
#include "../src/core/persist.h"

static void test_fixed(void) {
    printf("[TEST] Ponto Fixo Q8.8...\n");
    fix16_t a = fix_from_float(1.5f);
    fix16_t b = fix_from_float(2.0f);
    fix16_t c = fix_mul(a, b);
    float fc = fix_to_float(c);
    assert(fabsf(fc - 3.0f) < 0.01f);

    fix16_t d = fix_div(c, b);
    float fd = fix_to_float(d);
    assert(fabsf(fd - 1.5f) < 0.01f);

    /* Saturação */
    int16_t max16 = sat_add16(32000, 2000);
    assert(max16 == 32767);
    int16_t min16 = sat_sub16(-32000, 2000);
    assert(min16 == -32768);

    int8_t max8 = sat_add8(120, 20);
    assert(max8 == 127);
    int8_t min8 = sat_add8(-120, -20);
    assert(min8 == -128);

    /* Decaimento por shift */
    int16_t v = 256;
    int16_t v_decay = fix_decay_shift(v, 3); /* 256 - 32 = 224 */
    assert(v_decay == 224);

    printf("  -> Ponto Fixo: OK\n");
}

static void test_arena(void) {
    printf("[TEST] Arena de Memória Linear...\n");
    size_t cap = 1024 * 64; /* 64 KB */
    Arena *arena = arena_create(cap);
    assert(arena != NULL);
    assert(arena_capacity(arena) == cap);
    assert(arena_used(arena) == 0);

    /* Alocações com alinhamentos variados */
    void *p1 = arena_alloc(arena, 100);
    assert(p1 != NULL);
    assert(((uintptr_t)p1 % 16) == 0);

    void *p2 = arena_alloc_aligned(arena, 200, 64);
    assert(p2 != NULL);
    assert(((uintptr_t)p2 % 64) == 0);

    void *p3 = arena_alloc_zero(arena, 50);
    assert(p3 != NULL);
    uint8_t *b = (uint8_t *)p3;
    for (int i = 0; i < 50; ++i) {
        assert(b[i] == 0);
    }

    size_t used = arena_used(arena);
    assert(used > 350);

    arena_reset(arena);
    assert(arena_used(arena) == 0);

    /* Tentar alocar além da capacidade */
    void *huge = arena_alloc(arena, cap + 10);
    assert(huge == NULL);

    arena_destroy(arena);
    printf("  -> Arena: OK\n");
}

static void test_rng(void) {
    printf("[TEST] RNG xoshiro128** Determinístico...\n");
    RngState rng1, rng2;
    rng_seed(&rng1, 9999);
    rng_seed(&rng2, 9999);

    for (int i = 0; i < 1000; ++i) {
        uint32_t v1 = rng_u32(&rng1);
        uint32_t v2 = rng_u32(&rng2);
        assert(v1 == v2); /* Determinismo estrito */

        float f1 = rng_f32(&rng1);
        float f2 = rng_f32(&rng2);
        assert(f1 == f2);
        assert(f1 >= 0.0f && f1 < 1.0f);

        int32_t r1 = rng_range(&rng1, 10, 50);
        int32_t r2 = rng_range(&rng2, 10, 50);
        assert(r1 == r2);
        assert(r1 >= 10 && r1 <= 50);
    }
    printf("  -> RNG: OK\n");
}

static void test_persist_v2(void) {
    printf("[TEST] Persistência v2 com CRC32, Escrita Atômica e Fuzz Test...\n");
    const char *test_path = "test_brain_atomic.bin";
    remove(test_path);

    /* 1. Teste de gravação e leitura íntegra */
    float fake_weights[100];
    for (int i = 0; i < 100; ++i) fake_weights[i] = (float)i * 0.123f;

    bool saved = persist_save_atomic(test_path, 1024, fake_weights, sizeof(fake_weights));
    assert(saved);

    float loaded_weights[100];
    size_t bytes_read = 0;
    bool loaded = persist_load_safe(test_path, 1024, loaded_weights, sizeof(loaded_weights), &bytes_read);
    assert(loaded);
    assert(bytes_read == sizeof(fake_weights));
    for (int i = 0; i < 100; ++i) {
        assert(fake_weights[i] == loaded_weights[i]);
    }

    /* 2. Teste de rejeição por incompatibilidade de neurônios */
    bool wrong_neurons = persist_load_safe(test_path, 2048, loaded_weights, sizeof(loaded_weights), NULL);
    assert(!wrong_neurons);

    /* 3. Teste de detecção de corrupção de payload (CRC32 mismatch) */
    FILE *f = fopen(test_path, "r+b");
    assert(f != NULL);
    fseek(f, sizeof(PersistHeader) + 10, SEEK_SET);
    uint8_t corrupt_byte = 0xFF;
    fwrite(&corrupt_byte, 1, 1, f);
    fclose(f);

    bool corrupt_loaded = persist_load_safe(test_path, 1024, loaded_weights, sizeof(loaded_weights), NULL);
    assert(!corrupt_loaded); /* Rejeitado com sucesso pelo CRC32 */

    /* 4. Fuzz test: 100 arquivos com bytes e tamanhos aleatórios não devem crashar */
    RngState rng;
    rng_seed(&rng, 0x12345678);
    for (int run = 0; run < 100; ++run) {
        size_t fuzz_size = (size_t)(rng_u32(&rng) % 1024);
        uint8_t fuzz_buf[1024];
        for (size_t b = 0; b < fuzz_size; ++b) {
            fuzz_buf[b] = (uint8_t)(rng_u32(&rng) & 0xFF);
        }

        FILE *ff = fopen(test_path, "wb");
        if (ff) {
            fwrite(fuzz_buf, 1, fuzz_size, ff);
            fclose(ff);
        }

        uint8_t out_buf[2048];
        /* persist_load_safe deve lidar suavemente com lixo binário e retornar false sem abortar/crash */
        bool fuzz_res = persist_load_safe(test_path, 1024, out_buf, sizeof(out_buf), NULL);
        (void)fuzz_res;
    }

    remove(test_path);
    char tmp_path[512];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", test_path);
    remove(tmp_path);

    printf("  -> Persistência v2 & Fuzz Test: OK\n");
}

int main(void) {
    printf("=== EXECUÇÃO LOTE 0.1: FUNDAMENTOS DO NÚCLEO ===\n");
    test_fixed();
    test_arena();
    test_rng();
    test_persist_v2();
    printf("=== LOTE 0.1 APROVADO COM SUCESSO ===\n");
    return 0;
}
