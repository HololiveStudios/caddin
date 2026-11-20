#include "config_rules.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

static void trim_str(char *s) {
    if (!s) return;
    char *start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);

    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[len - 1] = '\0';
        len--;
    }
}

static bool str_contains_case_insensitive(const char *haystack, const char *needle) {
    if (!haystack || !needle) return false;
    if (!needle[0]) return true;

    size_t nlen = strlen(needle);
    size_t hlen = strlen(haystack);
    if (nlen > hlen) return false;

    for (size_t i = 0; i <= hlen - nlen; ++i) {
        size_t j = 0;
        while (j < nlen && tolower((unsigned char)haystack[i + j]) == tolower((unsigned char)needle[j])) {
            j++;
        }
        if (j == nlen) return true;
    }
    return false;
}

void config_rules_init(ConfigRules *cr) {
    if (!cr) return;
    memset(cr, 0, sizeof(*cr));

    /* Regras padrão de fábrica */
    /* 1. Modo Noturno / Madrugada: Se passar das 23h ou antes das 6h e estiver no terminal/code, fica calmo/dorminhoco */
    CustomRule *r = &cr->rules[cr->num_rules++];
    snprintf(r->name, sizeof(r->name), "late_night_focus");
    r->hour_min = 23;
    r->hour_max = 6;
    snprintf(r->action, sizeof(r->action), "inject_ser");
    r->param = 0.05f;
    r->cooldown_s = 60.0;

    /* Apps ignorados por padrão para privacidade */
    snprintf(cr->ignored_apps[cr->num_ignored++], RULE_STR_LEN, "keepass");
    snprintf(cr->ignored_apps[cr->num_ignored++], RULE_STR_LEN, "bitwarden");
    snprintf(cr->ignored_apps[cr->num_ignored++], RULE_STR_LEN, "1password");
    snprintf(cr->ignored_apps[cr->num_ignored++], RULE_STR_LEN, "private window");
    snprintf(cr->ignored_apps[cr->num_ignored++], RULE_STR_LEN, "incognito");
}

bool config_rules_load(ConfigRules *cr, const char *path) {
    if (!cr || !path) return false;
    FILE *f = fopen(path, "r");
    if (!f) return false;

    char line[256];
    char section[32] = {0};
    CustomRule *curr = NULL;

    while (fgets(line, sizeof(line), f)) {
        trim_str(line);
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';') continue;

        if (line[0] == '[' && line[strlen(line) - 1] == ']') {
            line[strlen(line) - 1] = '\0';
            const char *sec = line + 1;
            if (strncmp(sec, "rule", 4) == 0) {
                if (cr->num_rules < MAX_RULES) {
                    curr = &cr->rules[cr->num_rules++];
                    memset(curr, 0, sizeof(*curr));
                    curr->hour_min = -1;
                    curr->hour_max = -1;
                    curr->cooldown_s = 10.0;
                    if (sec[4] == ':' || sec[4] == '.') {
                        snprintf(curr->name, sizeof(curr->name), "%.63s", sec + 5);
                        trim_str(curr->name);
                    } else {
                        snprintf(curr->name, sizeof(curr->name), "rule_%zu", cr->num_rules);
                    }
                } else {
                    curr = NULL;
                }
                snprintf(section, sizeof(section), "rule");
            } else if (strcmp(sec, "privacy") == 0 || strcmp(sec, "ignore") == 0) {
                curr = NULL;
                snprintf(section, sizeof(section), "privacy");
            }
            continue;
        }

        if (strcmp(section, "privacy") == 0) {
            if (cr->num_ignored < MAX_IGNORED_APPS) {
                snprintf(cr->ignored_apps[cr->num_ignored++], RULE_STR_LEN, "%s", line);
            }
            continue;
        }

        if (curr && strcmp(section, "rule") == 0) {
            char *eq = strchr(line, '=');
            if (!eq) continue;
            *eq = '\0';
            char *key = line;
            char *val = eq + 1;
            trim_str(key);
            trim_str(val);

            if (strcmp(key, "window") == 0 || strcmp(key, "title") == 0) {
                snprintf(curr->window_pattern, sizeof(curr->window_pattern), "%s", val);
            } else if (strcmp(key, "action") == 0) {
                snprintf(curr->action, sizeof(curr->action), "%s", val);
            } else if (strcmp(key, "hour_min") == 0) {
                curr->hour_min = atoi(val);
            } else if (strcmp(key, "hour_max") == 0) {
                curr->hour_max = atoi(val);
            } else if (strcmp(key, "param") == 0) {
                curr->param = (float)atof(val);
            } else if (strcmp(key, "cooldown") == 0) {
                curr->cooldown_s = atof(val);
            }
        }
    }

    fclose(f);
    return true;
}

bool config_rules_is_ignored(const ConfigRules *cr, const char *title) {
    if (!cr || !title || !title[0]) return false;
    for (size_t i = 0; i < cr->num_ignored; ++i) {
        if (cr->ignored_apps[i][0] && str_contains_case_insensitive(title, cr->ignored_apps[i])) {
            return true;
        }
    }
    return false;
}

static bool check_hour_match(int hour, int h_min, int h_max) {
    if (h_min < 0 && h_max < 0) return true;
    if (h_min <= h_max) {
        return (hour >= h_min && hour <= h_max);
    }
    /* Intervalo cruzando a meia-noite (ex: 22h até 6h) */
    return (hour >= h_min || hour <= h_max);
}

void config_rules_evaluate(ConfigRules *cr,
                           const char *window_title,
                           int current_hour,
                           double now_s,
                           RuleActionCallback callback,
                           void *user_data) {
    if (!cr || !callback) return;

    for (size_t i = 0; i < cr->num_rules; ++i) {
        CustomRule *r = &cr->rules[i];
        if (!r->action[0]) continue;

        /* Checa cooldown */
        if (now_s - r->last_trigger_t < r->cooldown_s) continue;

        /* Checa janela se especificada */
        if (r->window_pattern[0] && (!window_title || !str_contains_case_insensitive(window_title, r->window_pattern))) {
            continue;
        }

        /* Checa horário */
        if (!check_hour_match(current_hour, r->hour_min, r->hour_max)) {
            continue;
        }

        /* Regra disparada! */
        r->last_trigger_t = now_s;
        callback(r->action, r->param, user_data);
    }
}
