/* sense_heatmap.c — mapa de calor 8×8 do cursor */
#include "sense_heatmap.h"
#include <string.h>

void sense_heatmap_init(SenseHeatmap *h, int screen_w, int screen_h) {
    memset(h, 0, sizeof(*h));
    h->scr_w      = screen_w > 0 ? screen_w : 1920;
    h->scr_h      = screen_h > 0 ? screen_h : 1080;
    h->decay_rate = 0.015;   /* 1.5% por segundo — memória longa */
    h->last_decay = 0.0;
}

void sense_heatmap_register(SenseHeatmap *h, int cursor_x, int cursor_y) {
    if (cursor_x < 0 || cursor_y < 0) return;
    int col = (cursor_x * HEATMAP_COLS) / h->scr_w;
    int row = (cursor_y * HEATMAP_ROWS) / h->scr_h;
    if (col < 0) col = 0;
    if (col >= HEATMAP_COLS) col = HEATMAP_COLS - 1;
    if (row < 0) row = 0;
    if (row >= HEATMAP_ROWS) row = HEATMAP_ROWS - 1;

    fix16_t v = h->cells[row][col];
    v += fix_from_float(0.005f);  /* +0.5% por frame */
    if (v > FIX_ONE) v = FIX_ONE;
    h->cells[row][col] = v;
}

void sense_heatmap_decay(SenseHeatmap *h, double now_sec) {
    if (h->last_decay == 0.0) { h->last_decay = now_sec; return; }
    double elapsed = now_sec - h->last_decay;
    if (elapsed < 0.5) return;  /* Atualiza a cada 0.5s */
    h->last_decay = now_sec;

    fix16_t decay = fix_from_float((float)(h->decay_rate * elapsed));
    for (int r = 0; r < HEATMAP_ROWS; r++) {
        for (int c = 0; c < HEATMAP_COLS; c++) {
            fix16_t v = h->cells[r][c];
            v = v > decay ? (v - decay) : 0;
            h->cells[r][c] = v;
        }
    }
}

void sense_heatmap_coldest(const SenseHeatmap *h, int *out_col, int *out_row) {
    int best_c = 0, best_r = 0;
    fix16_t best_v = FIX_ONE + 1;
    for (int r = 0; r < HEATMAP_ROWS; r++) {
        for (int c = 0; c < HEATMAP_COLS; c++) {
            if (h->cells[r][c] < best_v) {
                best_v = h->cells[r][c];
                best_c = c; best_r = r;
            }
        }
    }
    if (out_col) *out_col = best_c;
    if (out_row) *out_row = best_r;
}

void sense_heatmap_hottest(const SenseHeatmap *h, int *out_col, int *out_row) {
    int best_c = 0, best_r = 0;
    fix16_t best_v = -1;
    for (int r = 0; r < HEATMAP_ROWS; r++) {
        for (int c = 0; c < HEATMAP_COLS; c++) {
            if (h->cells[r][c] > best_v) {
                best_v = h->cells[r][c];
                best_c = c; best_r = r;
            }
        }
    }
    if (out_col) *out_col = best_c;
    if (out_row) *out_row = best_r;
}

void sense_heatmap_cell_to_px(const SenseHeatmap *h, int col, int row,
                               int *out_x, int *out_y) {
    if (out_x) *out_x = (col * h->scr_w / HEATMAP_COLS) + (h->scr_w / HEATMAP_COLS / 2);
    if (out_y) *out_y = (row * h->scr_h / HEATMAP_ROWS) + (h->scr_h / HEATMAP_ROWS / 2);
}
