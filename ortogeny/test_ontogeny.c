/* test_ontogeny.c — gcc -std=c11 -Wall -Wextra -O2 ontogeny.c test_ontogeny.c */
#include "ontogeny.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails = 0, checks = 0;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FALHOU %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)

static uint32_t lcg_s = 12345;
static uint32_t rnd(void) { lcg_s = lcg_s * 1664525u + 1013904223u; return lcg_s >> 8; }

static Ontogeny O;   /* ~50 KB: estático */

static void fill(q88_t *a, q88_t v) { for (int i = 0; i < ONTO_POOLS; i++) a[i] = v; }

static void test_brake_and_stages(void)
{
    OntoParams p = onto_params_default(); p.target_s = 3600;
    onto_init(&O, &p);
    q88_t err[ONTO_POOLS], act[ONTO_POOLS];
    fill(err, 0); fill(act, 256);
    for (int i = 100; i < ONTO_POOLS; i++) act[i] = 0;          /* pools privados (mic desligado) */
    int visited = 0, last = 0;
    for (int s = 0; s < 3800; s++) {
        onto_tick_1hz(&O, err, act, 0);
        CHECK(O.stage >= last);                                  /* nunca regride sozinho */
        last = O.stage; visited |= 1 << O.stage;
    }
    CHECK(visited == 0xF);                                       /* passou pelos 4 estágios */
    CHECK(O.pool[0].brake > 0xF0000000u);
    CHECK(O.pool[110].brake == 0);                               /* privação: janela segue aberta */
    CHECK(O.stage == ONTO_VETERANO);
    CHECK(onto_lr_scale(&O, 0) == p.lr_floor);                   /* adulto: piso, não zero */
    CHECK(onto_lr_scale(&O, 110) == 256);                        /* pool privado: aprende 100% */

    /* reabertura e histerese */
    O.pool[0].brake = 0xC0000000u;
    fill(err, 200);
    for (int s = 0; s < 1000; s++) onto_tick_1hz(&O, err, act, 0);
    CHECK(O.pool[0].brake < 0xC0000000u && O.pool[0].brake > 0);
    fill(err, 80);                                               /* entre theta_lo e theta_hi */
    for (int s = 0; s < 100; s++) onto_tick_1hz(&O, err, act, 0);
    uint32_t b0 = O.pool[0].brake;
    for (int s = 0; s < 500; s++) onto_tick_1hz(&O, err, act, 0);
    CHECK(O.pool[0].brake == b0);                                /* zona morta: nada muda */
}

static void test_crystal_math(void)
{
    q88_t w[36 * 16], dq[36 * 16], me; int8_t q[36 * 16]; uint16_t sc;
    for (int i = 0; i < 36 * 16; i++) w[i] = (q88_t)((int)(rnd() % 769) - 384);   /* ±1.5 */
    int ok = crystal_quantize(w, 36 * 16, q, &sc, &me, 2);
    CHECK(ok && me <= sc / 2 && sc <= 4);
    crystal_dequantize(q, 36 * 16, sc, dq);

    uint64_t spikes = ((uint64_t)rnd() << 20 ^ rnd()) & ((1ULL << 36) - 1);
    int32_t acc[16] = {0}, ref[16] = {0}, deq[16] = {0};
    crystal_propagate(q, sc, 36, 16, spikes, acc);
    int ns = 0;
    for (int i = 0; i < 36; i++) if (spikes >> i & 1) {
        ns++;
        for (int j = 0; j < 16; j++) { ref[j] += w[i * 16 + j]; deq[j] += dq[i * 16 + j]; }
    }
    for (int j = 0; j < 16; j++) {
        CHECK(acc[j] == deq[j]);                                 /* exato vs. dequantizado */
        int d = acc[j] - ref[j]; if (d < 0) d = -d;
        CHECK(d <= ns * (sc / 2));                               /* erro limitado por n*scale/2 */
    }
    for (int i = 0; i < 36 * 16; i++) w[i] = (q88_t)((int)(rnd() % 40001) - 20000);
    ok = crystal_quantize(w, 36 * 16, q, &sc, &me, 2);
    CHECK(!ok && me > 8);                                        /* pesos grandes: rejeita */

    q88_t a[4] = {100, 200, 300, 400}, b[4] = {101, 199, 300, 402};
    CHECK(onto_equiv_check(a, b, 4, 2) && !onto_equiv_check(a, b, 4, 1));
    CHECK(onto_equiv_spikes(1000, 1015, 20) && !onto_equiv_spikes(1000, 1100, 20));
}

static void sleep_with_good_notes(Ontogeny *o, int proj, int n)
{
    for (int i = 0; i < n; i++) {
        onto_proj_note(&o->proj[proj], 2000, 10, 80);
        onto_sleep_consolidate(o);
    }
}

static void test_state_machine(void)
{
    onto_init(&O, NULL);
    int k = onto_add_proj(&O, 3, 40, 576);
    CHECK(k == 0);
    O.pool[3].brake = O.pool[40].brake = 0xB0000000u;            /* janelas fechadas */

    sleep_with_good_notes(&O, k, 4);
    CHECK(O.proj[k].state == PROJ_PLASTIC);
    sleep_with_good_notes(&O, k, 1);
    CHECK(O.proj[k].state == PROJ_CANDIDATE);

    onto_commit(&O, k, 0);                                       /* validação falhou: rollback */
    CHECK(O.proj[k].state == PROJ_PLASTIC && O.proj[k].cooldown == 3);
    sleep_with_good_notes(&O, k, 3 + 4);                         /* quarentena + 4 ciclos */
    CHECK(O.proj[k].state == PROJ_PLASTIC);
    sleep_with_good_notes(&O, k, 1);
    CHECK(O.proj[k].state == PROJ_CANDIDATE);
    onto_commit(&O, k, 1);
    CHECK(O.proj[k].state == PROJ_CRYSTAL);

    /* instabilidade zera a contagem */
    onto_init(&O, NULL); onto_add_proj(&O, 3, 40, 576);
    O.pool[3].brake = O.pool[40].brake = 0xB0000000u;
    sleep_with_good_notes(&O, 0, 4);
    onto_proj_note(&O.proj[0], 2000, 100000, 80); onto_sleep_consolidate(&O);
    CHECK(O.proj[0].stab == 0);
    sleep_with_good_notes(&O, 0, 1);
    CHECK(O.proj[0].state == PROJ_PLASTIC);

    /* janela ainda aberta: nunca vira candidata, mesmo estável e útil */
    onto_init(&O, NULL); onto_add_proj(&O, 3, 40, 576);
    sleep_with_good_notes(&O, 0, 20);
    CHECK(O.proj[0].state == PROJ_PLASTIC && O.proj[0].stab >= 5);

    /* descongelamento com histerese */
    onto_init(&O, NULL); onto_add_proj(&O, 3, 40, 576);
    O.proj[0].state = PROJ_CRYSTAL;
    q88_t err[ONTO_POOLS], act[ONTO_POOLS]; fill(err, 0); fill(act, 256);
    err[40] = 100;                                               /* abaixo de err_melt: nada */
    for (int s = 0; s < 2000; s++) onto_tick_1hz(&O, err, act, 200);
    CHECK(O.proj[0].state == PROJ_CRYSTAL);
    err[40] = 256;
    for (int s = 0; s < 300; s++) onto_tick_1hz(&O, err, act, 200);
    CHECK(O.proj[0].state == PROJ_CRYSTAL);                      /* ainda não: precisa persistir */
    for (int s = 0; s < 600; s++) onto_tick_1hz(&O, err, act, 200);
    CHECK(O.proj[0].state == PROJ_PLASTIC && O.proj[0].restore == 1);
}

static void test_imprint(void)
{
    Imprint im; imprint_reset(&im);
    q88_t dono[8] = {200,180,150,120,100,80,60,40}, estranho[8] = {20,30,40,60,100,150,200,230};
    imprint_observe(&im, dono, 1);
    CHECK(im.state == 1 && imprint_match(&im, dono) >= 250 && imprint_match(&im, estranho) < IMPRINT_MATCH_MIN);
    imprint_observe(&im, estranho, 1);                           /* visitante não conta */
    CHECK(im.sessions == 1);
    imprint_observe(&im, dono, 1); imprint_observe(&im, dono, 1);
    CHECK(im.sessions == 3 && im.state == 1);
    imprint_sleep(&im);
    CHECK(im.state == 2);
}

static void test_persist(void)
{
    static uint8_t buf[70000], onto_b[30000];
    uint8_t snn[100], impr_b[32], v2[64];
    for (int i = 0; i < 100; i++) snn[i] = (uint8_t)(i * 7);
    for (int i = 0; i < 64; i++) v2[i] = (uint8_t)(i * 3 + 1);

    onto_init(&O, NULL); onto_add_proj(&O, 3, 40, 576); onto_add_proj(&O, 5, 41, 288);
    O.stage = ONTO_ADULTO; O.awake_s = 123456; O.sleeps = 77;
    O.pool[7].brake = 0xDEADBEEFu; O.proj[1].state = PROJ_CRYSTAL; O.proj[1].melt_cnt = 42;
    size_t on = onto_serialize(&O, onto_b, sizeof onto_b);
    Imprint im; imprint_reset(&im);
    q88_t dono[8] = {200,180,150,120,100,80,60,40};
    imprint_observe(&im, dono, 1);
    size_t in = imprint_serialize(&im, impr_b, sizeof impr_b);
    CHECK(on > 0 && in == 19);

    size_t off = brain3_begin(buf, sizeof buf);
    off = brain3_put(buf, sizeof buf, off, TAG_SNN, snn, sizeof snn);
    off = brain3_put(buf, sizeof buf, off, BRAIN3_TAG('F','U','T','R'), "futuro", 6);  /* tag desconhecida */
    off = brain3_put(buf, sizeof buf, off, TAG_ONTO, onto_b, (uint32_t)on);
    off = brain3_put(buf, sizeof buf, off, TAG_IMPR, impr_b, (uint32_t)in);
    CHECK(off > 0);
    brain3_finish(buf, off, 4);

    Brain3Index ix;
    CHECK(brain3_parse(buf, off, &ix) == 0);
    CHECK(ix.snn.status == 1 && ix.onto.status == 1 && ix.impr.status == 1 && ix.n_unknown == 1);

    static Ontogeny R;
    onto_init(&R, NULL);
    CHECK(onto_deserialize(&R, ix.onto.ptr, ix.onto.len) == 1);
    CHECK(R.stage == ONTO_ADULTO && R.awake_s == 123456 && R.sleeps == 77 && R.n_proj == 2);
    CHECK(R.pool[7].brake == 0xDEADBEEFu && R.proj[1].state == PROJ_CRYSTAL && R.proj[1].melt_cnt == 42);
    Imprint im2; CHECK(imprint_deserialize(&im2, ix.impr.ptr, ix.impr.len) && im2.state == 1);

    /* corrupção suave: ONTO ruim, cérebro (SNN) preservado */
    uint8_t *copy = malloc(off); memcpy(copy, buf, off);
    Brain3Index c; brain3_parse(copy, off, &c);
    copy[(c.onto.ptr - copy) + 10] ^= 0x55;
    CHECK(brain3_parse(copy, off, &c) == 0 && c.onto.status == 2 && c.snn.status == 1);
    /* corrupção fatal: SNN */
    memcpy(copy, buf, off); brain3_parse(copy, off, &c);
    copy[(c.snn.ptr - copy) + 3] ^= 0x01;
    CHECK(brain3_parse(copy, off, &c) == -3);
    memcpy(copy, buf, off); copy[1] ^= 0xFF;
    CHECK(brain3_parse(copy, off, &c) == -1);
    memcpy(copy, buf, off);
    CHECK(brain3_parse(copy, off - 1, &c) == -2);
    free(copy);

    /* dado inválido no ONTO => reset seguro */
    onto_b[0] = 9;
    CHECK(onto_deserialize(&R, onto_b, on) == 0 && R.stage == ONTO_FILHOTE && R.n_proj == 0);

    /* migração v2 -> v3 */
    size_t m = brain3_wrap_v2(buf, sizeof buf, v2, sizeof v2);
    CHECK(m > 0 && brain3_parse(buf, m, &ix) == 0);
    CHECK(ix.snn.len == 64 && memcmp(ix.snn.ptr, v2, 64) == 0 && ix.onto.status == 0);
    onto_init_migrated(&R, NULL);
    CHECK(R.stage == ONTO_JUVENIL && R.pool[0].brake == 0x80000000u);
}

int main(void)
{
    test_brake_and_stages();
    test_crystal_math();
    test_state_machine();
    test_imprint();
    test_persist();
    printf("%d verificações, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
