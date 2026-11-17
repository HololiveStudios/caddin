#include "lexicon.h"
#include <string.h>

static uint32_t hash_str(const char *str) {
    uint32_t h = 5381;
    int c;
    while ((c = (unsigned char)*str++)) {
        h = ((h << 5) + h) + c; /* hash * 33 + c */
    }
    return h;
}

void lexicon_init(Lexicon *lex) {
    if (!lex) return;
    memset(lex, 0, sizeof(Lexicon));
    lex->count = 0;
}

LexiconEntry *lexicon_lookup(Lexicon *lex, const char *word) {
    if (!lex || !word) return NULL;
    uint32_t h = hash_str(word);
    for (uint32_t i = 0; i < lex->count; ++i) {
        if (lex->entries[i].word_hash == h && strncmp(lex->entries[i].label, word, 23) == 0) {
            return &lex->entries[i];
        }
    }
    return NULL;
}

LexiconEntry *lexicon_observe_word(Lexicon *lex, const char *word, fix16_t current_valence, fix16_t current_arousal, int context_tag) {
    if (!lex || !word || word[0] == '\0') return NULL;

    LexiconEntry *entry = lexicon_lookup(lex, word);
    if (!entry) {
        if (lex->count >= MAX_LEXICON_ENTRIES) {
            /* Tabela cheia: substitui a entrada de menor frequência */
            uint32_t min_idx = 0;
            uint16_t min_freq = 0xFFFF;
            for (uint32_t i = 0; i < lex->count; ++i) {
                if (lex->entries[i].frequency < min_freq) {
                    min_freq = lex->entries[i].frequency;
                    min_idx = i;
                }
            }
            entry = &lex->entries[min_idx];
        } else {
            entry = &lex->entries[lex->count++];
        }

        entry->word_hash = hash_str(word);
        strncpy(entry->label, word, 23);
        entry->label[23] = '\0';
        entry->frequency = 0;
        entry->confidence = 40;
        entry->valence = current_valence;
        entry->arousal = current_arousal;
        entry->context_tag = context_tag;
        entry->needs_clarification = false;
    }

    /* Atualização cumulativa */
    entry->frequency++;

    /* Média móvel exponencial da valência e excitação associadas */
    entry->valence = (fix16_t)((entry->valence * 3 + current_valence) / 4);
    entry->arousal = (fix16_t)((entry->arousal * 3 + current_arousal) / 4);

    /* Confiança cresce se a associação de contexto for coerente */
    if (entry->context_tag == context_tag) {
        if (entry->confidence < 240) entry->confidence += 15;
    } else {
        /* Contexto conflitante: reduz confiança e marca curiosidade dirigida */
        if (entry->confidence > 20) entry->confidence -= 10;
        if (entry->frequency > 3 && entry->confidence < 80) {
            entry->needs_clarification = true;
        }
    }

    return entry;
}

void lexicon_consolidate_sleep(Lexicon *lex) {
    if (!lex) return;

    /* No sono profundo, remove entradas isoladas ruidosas (freq == 1) */
    uint32_t write_idx = 0;
    for (uint32_t i = 0; i < lex->count; ++i) {
        if (lex->entries[i].frequency > 1 || lex->entries[i].confidence > 60) {
            if (write_idx != i) {
                lex->entries[write_idx] = lex->entries[i];
            }
            write_idx++;
        }
    }
    lex->count = write_idx;
}
