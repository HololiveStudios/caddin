/* behavior_explore.c — curiosidade e exploração territorial */
#define _POSIX_C_SOURCE 200809L
#include "behavior_explore.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

void behavior_explore_init(BehaviorExplore *e) {
    memset(e, 0, sizeof(*e));
    e->state       = EXPLORE_REST;
    e->target_x    = -1;
    e->target_y    = -1;
    e->home_col    = 7;  /* canto inferior direito padrão */
    e->home_row    = 7;
    e->home_strength = fix_from_float(0.1f);
    e->curiosity   = fix_from_float(0.5f);
}

void behavior_explore_update_home(BehaviorExplore *e, const SenseHeatmap *heatmap) {
    int hcol, hrow;
    sense_heatmap_hottest(heatmap, &hcol, &hrow);
    if (heatmap->cells[hrow][hcol] > fix_from_float(0.3f)) {
        /* Território emergente: célula mais visitada vira "lar" */
        e->home_col  = hcol;
        e->home_row  = hrow;
        fix16_t gain = fix_from_float(0.001f);
        e->home_strength += gain;
        if (e->home_strength > FIX_ONE) e->home_strength = FIX_ONE;
    }
}

ExploreState behavior_explore_update(BehaviorExplore *e,
                                     const SenseHeatmap *heatmap,
                                     int pet_x, int pet_y,
                                     fix16_t curiosity_level,
                                     double now_sec) {
    e->curiosity = curiosity_level;

    /* Verifica se o estado atual terminou */
    if (e->state != EXPLORE_REST) {
        if (now_sec - e->state_start >= e->state_duration) {
            e->state    = EXPLORE_REST;
            e->target_x = -1;
            e->target_y = -1;
        } else {
            return e->state;
        }
    }

    /* Decisão de exploração: threshold baseado na curiosidade */
    float cur = fix_to_float(curiosity_level);
    int r = rand() % 1000;
    int threshold = (int)(cur * 300.0f);  /* 0..300 de 1000 */

    if (r < threshold) {
        /* Vai para região inexplorada */
        int cold_col, cold_row;
        sense_heatmap_coldest(heatmap, &cold_col, &cold_row);
        sense_heatmap_cell_to_px(heatmap, cold_col, cold_row,
                                 &e->target_x, &e->target_y);
        e->state          = EXPLORE_WANDER;
        e->state_start    = now_sec;
        e->state_duration = 3.0 + (rand() % 50) * 0.1;
    } else if (r < threshold + 100) {
        /* Reclama território: vai para célula mais quente */
        sense_heatmap_cell_to_px(heatmap, e->home_col, e->home_row,
                                 &e->target_x, &e->target_y);
        e->state          = EXPLORE_CLAIM;
        e->state_start    = now_sec;
        e->state_duration = 5.0 + (rand() % 100) * 0.1;
    } else if (r < threshold + 150 && cur > 0.3f) {
        /* Olha para borda */
        e->target_x = pet_x;
        e->target_y = pet_y;
        e->state          = EXPLORE_PEEK;
        e->state_start    = now_sec;
        e->state_duration = 1.5;
    }

    return e->state;
}

void behavior_explore_get_target(const BehaviorExplore *e, int *out_x, int *out_y) {
    if (out_x) *out_x = e->target_x;
    if (out_y) *out_y = e->target_y;
}
