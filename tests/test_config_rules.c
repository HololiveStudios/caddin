#include "config_rules.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static int callback_count = 0;
static char last_action[64] = {0};
static float last_param = 0.0f;

static void test_callback(const char *action, float param, void *user_data) {
    (void)user_data;
    callback_count++;
    snprintf(last_action, sizeof(last_action), "%s", action);
    last_param = param;
}

int main(void) {
    ConfigRules cr;
    config_rules_init(&cr);

    /* 1. Teste de privacidade padrão */
    assert(config_rules_is_ignored(&cr, "KeePassXC - Passwords.kdbx") == true);
    assert(config_rules_is_ignored(&cr, "Mozilla Firefox - Bitwarden Vault") == true);
    assert(config_rules_is_ignored(&cr, "Terminal - bash") == false);

    /* 2. Teste de arquivo de regras customizadas */
    const char *test_conf = "/tmp/test_caddin.conf";
    FILE *f = fopen(test_conf, "w");
    assert(f != NULL);
    fprintf(f, "# Arquivo de teste\n");
    fprintf(f, "[privacy]\n");
    fprintf(f, "secret_project\n");
    fprintf(f, "\n");
    fprintf(f, "[rule:compiler_focus]\n");
    fprintf(f, "window = gcc\n");
    fprintf(f, "action = react_curious\n");
    fprintf(f, "param = 0.8\n");
    fprintf(f, "cooldown = 5.0\n");
    fprintf(f, "\n");
    fprintf(f, "[rule:night_owl]\n");
    fprintf(f, "window = code\n");
    fprintf(f, "hour_min = 22\n");
    fprintf(f, "hour_max = 5\n");
    fprintf(f, "action = inject_da\n");
    fprintf(f, "param = 0.15\n");
    fprintf(f, "cooldown = 10.0\n");
    fclose(f);

    bool loaded = config_rules_load(&cr, test_conf);
    assert(loaded == true);
    (void)loaded;
    assert(config_rules_is_ignored(&cr, "My secret_project documentation") == true);

    /* 3. Teste de avaliação de regras */
    callback_count = 0;
    config_rules_evaluate(&cr, "Compiling main.c with gcc -O2", 14, 100.0, test_callback, NULL);
    assert(callback_count == 1);
    assert(strcmp(last_action, "react_curious") == 0);
    assert(last_param == 0.8f);

    /* Cooldown: não deve disparar novamente imediatamente */
    callback_count = 0;
    config_rules_evaluate(&cr, "Compiling main.c with gcc -O2", 14, 102.0, test_callback, NULL);
    assert(callback_count == 0);

    /* Após o cooldown (5s), dispara */
    config_rules_evaluate(&cr, "Compiling main.c with gcc -O2", 14, 106.0, test_callback, NULL);
    assert(callback_count == 1);

    /* Teste de horário noturno */
    callback_count = 0;
    /* 23h: deve casar [22, 5] */
    config_rules_evaluate(&cr, "VS Code - main.c", 23, 200.0, test_callback, NULL);
    assert(callback_count == 1);
    assert(strcmp(last_action, "inject_da") == 0);

    /* 15h: fora do horário, não deve disparar */
    callback_count = 0;
    config_rules_evaluate(&cr, "VS Code - main.c", 15, 300.0, test_callback, NULL);
    assert(callback_count == 0);

    remove(test_conf);
    printf("Todos os testes de config_rules passaram com sucesso!\n");
    return 0;
}
