/* sense_heatmap.h — mapa de calor 8×8 do cursor
 * Registra ocupação do cursor em células 8×8 da tela.
 * Decaimento exponencial slow. Permite detectar territórios e zonas inexploradas.
 */
#ifndef SENSE_HEATMAP_H
#define SENSE_HEATMAP_H

#include <stdint.h>
#include "../core/fixed.h"

#define HEATMAP_COLS 8
#define HEATMAP_ROWS 8

typedef struct {
    fix16_t cells[HEATMAP_ROWS][HEATMAP_COLS]; /* calor 0..FIX_ONE */
    int     scr_w, scr_h;    /* dimensões da tela em px */
    double  decay_rate;       /* por segundo, ex: 0.02 */
    double  last_decay;       /* timestamp do último decaimento */
} SenseHeatmap;

void    sense_heatmap_init(SenseHeatmap *h, int screen_w, int screen_h);
void    sense_heatmap_register(SenseHeatmap *h, int cursor_x, int cursor_y);
void    sense_heatmap_decay(SenseHeatmap *h, double now_sec);

/* Retorna a célula mais "fria" (menos explorada) — curiosidade/exploração */
void    sense_heatmap_coldest(const SenseHeatmap *h, int *out_col, int *out_row);
/* Retorna a célula mais "quente" (território favorito) */
void    sense_heatmap_hottest(const SenseHeatmap *h, int *out_col, int *out_row);
/* Converte célula para coordenadas centrais na tela */
void    sense_heatmap_cell_to_px(const SenseHeatmap *h, int col, int row,
                                  int *out_x, int *out_y);

#endif /* SENSE_HEATMAP_H */
