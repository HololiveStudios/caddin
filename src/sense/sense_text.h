#ifndef PET_SENSE_TEXT_H
#define PET_SENSE_TEXT_H

#include <stdbool.h>
#include <stddef.h>

#define SENSE_TEXT_MAX_LEN 256

typedef struct {
    char current_title[SENSE_TEXT_MAX_LEN];
    char last_word[64];
    bool has_new_word;
    float poll_timer;
} SenseText;

void sense_text_init(SenseText *st);

/* Atualiza leitura de texto da janela ativa a cada ~0.5s sem overhead */
void sense_text_update(SenseText *st, float dt, void *native_display, unsigned long active_window);

/* Retorna a última palavra textual observada no ambiente */
const char *sense_text_get_last_word(const SenseText *st);

/* Retorna true se houver nova palavra disponível para associação no léxico */
bool sense_text_has_new_word(SenseText *st);

#endif /* PET_SENSE_TEXT_H */
