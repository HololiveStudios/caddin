#ifndef PET_LANG_LEXICON_H
#define PET_LANG_LEXICON_H

#include "../core/fixed.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_LEXICON_ENTRIES 256

typedef struct {
    uint32_t word_hash;
    char     label[24];
    uint16_t frequency;
    uint8_t  confidence;  /* Q0.8 [0..255] */
    fix16_t  valence;     /* [-1.0 .. 1.0] */
    fix16_t  arousal;     /* [0.0 .. 1.0] */
    int      context_tag;
    bool     needs_clarification; /* Gera curiosidade "?" */
} LexiconEntry;

typedef struct {
    LexiconEntry entries[MAX_LEXICON_ENTRIES];
    uint32_t     count;
} Lexicon;

void          lexicon_init(Lexicon *lex);
LexiconEntry *lexicon_observe_word(Lexicon *lex, const char *word, fix16_t current_valence, fix16_t current_arousal, int context_tag);
LexiconEntry *lexicon_lookup(Lexicon *lex, const char *word);
void          lexicon_consolidate_sleep(Lexicon *lex);

#endif /* PET_LANG_LEXICON_H */
