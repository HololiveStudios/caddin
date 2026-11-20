#ifndef PET_SENSE_TEXT_H
#define PET_SENSE_TEXT_H

#include <stdbool.h>
#include <stddef.h>

#define SENSE_TEXT_MAX_LEN 256

typedef struct {
    char current_title[SENSE_TEXT_MAX_LEN];
    char last_word[64];
    bool has_new_word;
    bool has_error;
    bool has_success;
    float poll_timer;
} SenseText;

void sense_text_init(SenseText *st);

/* Atualiza leitura de texto da janela ativa a cada ~0.5s sem overhead */
void sense_text_update(SenseText *st, float dt, void *native_display, unsigned long active_window);

/* Retorna a última palavra textual observada no ambiente */
const char *sense_text_get_last_word(const SenseText *st);

/* Retorna o título da janela ativa observado */
const char *sense_text_get_title(const SenseText *st);

/* Retorna true se houver nova palavra disponível para associação no léxico */
bool sense_text_has_new_word(SenseText *st);

/* Retorna true se um erro na tela/janela foi detectado (consome o evento) */
bool sense_text_has_error(SenseText *st);

/* Retorna true se um sucesso/build passed na tela foi detectado (consome o evento) */
bool sense_text_has_success(SenseText *st);

#endif /* PET_SENSE_TEXT_H */
