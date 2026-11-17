/* sense_luma.h — luminosidade sob o pet (via XGetImage ou fallback)
 * Captura região 16x16 sob o corpo do pet e retorna brilho médio 0..1.
 */
#ifndef SENSE_LUMA_H
#define SENSE_LUMA_H

#include <stdbool.h>
#include "../core/fixed.h"

/* Opaco: a implementação inclui Display* e window internamente quando em Linux */
typedef struct SenseLuma SenseLuma;

SenseLuma *sense_luma_create(void *display_ptr, unsigned long root_window);
void       sense_luma_destroy(SenseLuma *s);

/* Captura e retorna brilho médio 0..1 sob (cx, cy) com tamanho side×side.
 * Retorna último valor se a captura falhar. */
fix16_t    sense_luma_sample(SenseLuma *s, int cx, int cy, int side);

#endif /* SENSE_LUMA_H */
