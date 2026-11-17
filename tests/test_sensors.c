#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "../src/core/fixed.h"
#include "../src/sense/sense_mouse.h"
#include "../src/sense/sense_scroll.h"
#include "../src/sense/sense_windows.h"

static void test_sense_mouse(void) {
    printf("[TEST] Sensor de Mouse, Aceleração, Atrito e Sacudida...\n");
    SenseMouse sm;
    sense_mouse_init(&sm);

    /* Movimento suave longe do pet */
    sense_mouse_update(&sm, 100, 100, 500, 500, 30);
    sense_mouse_update(&sm, 105, 102, 500, 500, 30);
    assert(!sm.is_contacting_body);
    assert(sm.current_speed > 0);

    /* Movimento tocando o corpo do pet (centro em 500,500 raio 30) */
    sense_mouse_update(&sm, 520, 520, 500, 500, 30);
    assert(sm.is_contacting_body);

    /* Sacudida violenta: reversões bruscas de direção */
    for (int i = 0; i < 10; ++i) {
        int x = (i % 2 == 0) ? 200 : 50;
        sense_mouse_update(&sm, x, 100, 500, 500, 30);
    }
    fix16_t shake = sense_mouse_get_shake_intensity(&sm);
    assert(shake > fix_from_float(1.0f));

    printf("  -> SenseMouse: OK\n");
}

static void test_sense_scroll(void) {
    printf("[TEST] Sensor de Vento de Rolagem (Scroll)...\n");
    SenseScroll ss;
    sense_scroll_init(&ss);

    assert(ss.air_velocity == 0);
    sense_scroll_register(&ss, 5); /* Rolou 5 cliques rápidos */
    assert(ss.air_velocity > 0);

    fix16_t wind_before = ss.air_velocity;
    for (int i = 0; i < 10; ++i) sense_scroll_step(&ss);
    assert(ss.air_velocity < wind_before); /* Vento decaiu suavemente */

    printf("  -> SenseScroll: OK\n");
}

static void test_sense_windows(void) {
    printf("[TEST] Sensor de Janelas (Galhos, Foco e Quina Perigosa)...\n");
    SenseWindows sw;
    sense_windows_init(&sw);

    /* Janela em x=100, y=100, w=800, h=600 */
    sense_windows_update(&sw, 1, 100, 100, 800, 600);
    assert(sense_windows_did_switch_focus(&sw));

    /* Pet pousado na barra de título */
    bool on_top = sense_windows_is_on_top_edge(&sw, 300, 105, 15);
    assert(on_top);

    /* Pet perto do botão fechar (quina superior direita: ~880, 115) */
    bool near_danger = sense_windows_is_near_danger_zone(&sw, 875, 110, 40);
    assert(near_danger);

    /* Pet longe da quina perigosa */
    near_danger = sense_windows_is_near_danger_zone(&sw, 200, 300, 40);
    assert(!near_danger);

    printf("  -> SenseWindows: OK\n");
}

int main(void) {
    printf("=== EXECUÇÃO FASE 3: SENSORES AVANÇADOS DO DESKTOP ===\n");
    test_sense_mouse();
    test_sense_scroll();
    test_sense_windows();
    printf("=== FASE 3 APROVADA COM SUCESSO ===\n");
    return 0;
}
