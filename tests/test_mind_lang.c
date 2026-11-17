#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "../src/core/fixed.h"
#include "../src/mind/workspace.h"
#include "../src/lang/lexicon.h"

static void test_global_workspace(void) {
    printf("[TEST] Espaço de Trabalho Global (Global Workspace) e Atenção...\n");
    GlobalWorkspace gw;
    workspace_init(&gw);

    /* Nenhum foco inicial */
    assert(workspace_get_attended_focus(&gw) == STIMULUS_NONE);

    /* Submeter toque suave do mouse */
    workspace_submit_candidate(&gw, STIMULUS_MOUSE_TOUCH, fix_from_float(0.8f), 10);
    assert(workspace_get_attended_focus(&gw) == STIMULUS_MOUSE_TOUCH);

    /* Submeter sacudida violenta com saliência muito maior: deve roubar o foco */
    workspace_submit_candidate(&gw, STIMULUS_VIOLENT_SHAKE, fix_from_float(2.0f), 15);
    assert(workspace_get_attended_focus(&gw) == STIMULUS_VIOLENT_SHAKE);

    /* Testar habituação: após repetidas sacudidas, a habituação sobe */
    for (int i = 0; i < 5; ++i) {
        workspace_submit_candidate(&gw, STIMULUS_VIOLENT_SHAKE, fix_from_float(2.0f), 20 + i);
    }
    assert(gw.habituation[(int)STIMULUS_VIOLENT_SHAKE] > 50);

    printf("  -> Global Workspace: OK\n");
}

static void test_emergent_lexicon(void) {
    printf("[TEST] Léxico Emergente, Associação Multimodal e Consolidação no Sono...\n");
    Lexicon lex;
    lexicon_init(&lex);

    /* Parear "carinho" repetidamente com afeto positivo (valência +0.8) */
    for (int i = 0; i < 5; ++i) {
        lexicon_observe_word(&lex, "carinho", fix_from_float(0.8f), fix_from_float(0.3f), 1);
    }
    LexiconEntry *e_carinho = lexicon_lookup(&lex, "carinho");
    assert(e_carinho != NULL);
    assert(e_carinho->valence > 0);
    assert(e_carinho->confidence > 80);

    /* Parear "susto" com afeto negativo (valência -0.9) */
    for (int i = 0; i < 5; ++i) {
        lexicon_observe_word(&lex, "susto", fix_from_float(-0.9f), fix_from_float(0.9f), 2);
    }
    LexiconEntry *e_susto = lexicon_lookup(&lex, "susto");
    assert(e_susto != NULL);
    assert(e_susto->valence < 0);

    /* Registrar ruído que aparece só uma vez */
    lexicon_observe_word(&lex, "ruido123", 0, 0, 0);
    assert(lexicon_lookup(&lex, "ruido123") != NULL);

    /* Sono: consolidação remove ruídos de frequência 1 */
    lexicon_consolidate_sleep(&lex);
    assert(lexicon_lookup(&lex, "ruido123") == NULL);
    assert(lexicon_lookup(&lex, "carinho") != NULL);

    printf("  -> Léxico Emergente e Sono: OK\n");
}

int main(void) {
    printf("=== EXECUÇÃO FASE 5: MENTE, ATENÇÃO E LINGUAGEM EMERGENTE ===\n");
    test_global_workspace();
    test_emergent_lexicon();
    printf("=== FASE 5 APROVADA COM SUCESSO ===\n");
    return 0;
}
