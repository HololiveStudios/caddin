#ifndef PET_CONFIG_RULES_H
#define PET_CONFIG_RULES_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define MAX_RULES 32
#define MAX_IGNORED_APPS 16
#define RULE_STR_LEN 64

typedef struct {
    char name[RULE_STR_LEN];
    char window_pattern[RULE_STR_LEN]; /* Case-insensitive substring match */
    int  hour_min;                     /* 0..23 ou -1 se desabilitado */
    int  hour_max;                     /* 0..23 ou -1 se desabilitado */
    char action[RULE_STR_LEN];         /* react_sad, react_happy, inject_ne, etc. */
    float param;                       /* Magnitude ou intensidade */
    double cooldown_s;                 /* Cooldown em segundos */
    double last_trigger_t;             /* Timestamp do último disparo */
} CustomRule;

typedef struct {
    CustomRule rules[MAX_RULES];
    size_t     num_rules;
    char       ignored_apps[MAX_IGNORED_APPS][RULE_STR_LEN];
    size_t     num_ignored;
} ConfigRules;

void config_rules_init(ConfigRules *cr);

/* Carrega arquivo de configuração (.caddinrc ou path customizado).
 * Retorna true se arquivo existia e foi lido, false se usou padrões. */
bool config_rules_load(ConfigRules *cr, const char *path);

/* Verifica se um título de janela ou app está na lista de privacidade */
bool config_rules_is_ignored(const ConfigRules *cr, const char *title);

typedef void (*RuleActionCallback)(const char *action, float param, void *user_data);

/* Avalia todas as regras contra o contexto atual e executa o callback nas ativadas */
void config_rules_evaluate(ConfigRules *cr,
                           const char *window_title,
                           int current_hour,
                           double now_s,
                           RuleActionCallback callback,
                           void *user_data);

#endif /* PET_CONFIG_RULES_H */
