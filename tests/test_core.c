#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "../src/core/fixed.h"
#include "../src/core/arena.h"
#include "../src/core/rng.h"

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

int main(void) {
    printf("=== EXECUÇÃO LOTE 0.1: FUNDAMENTOS DO NÚCLEO ===\n");
    test_fixed();
    test_arena();
    test_rng();
    printf("=== LOTE 0.1 APROVADO COM SUCESSO ===\n");
    return 0;
}
