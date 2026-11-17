/* behavior_explore.h — curiosidade e exploração territorial
 * Usa o mapa de calor 8×8 para gerar impulsos em direção a regiões inexploradas.
 * Implementa território de canto favorito emergente.
 */
#ifndef BEHAVIOR_EXPLORE_H
#define BEHAVIOR_EXPLORE_H

#include <stdbool.h>
#include "../core/fixed.h"
#include "../sense/sense_heatmap.h"

typedef enum {
    EXPLORE_REST = 0,       /* sem exploração ativa */
    EXPLORE_WANDER,         /* vagando para região fria */
    EXPLORE_CLAIM,          /* "marcando território" (fica parado em região quente) */
    EXPLORE_PEEK,           /* olha para borda curiosamente */
} ExploreState;

typedef struct {
    ExploreState state;
    int          target_x, target_y;   /* coordenadas-alvo em px */
    double       state_start;
    double       state_duration;
    fix16_t      curiosity;            /* nível de curiosidade 0..1 */
    int          home_col, home_row;   /* célula favorita (território) */
    fix16_t      home_strength;        /* força do vínculo com o lar */
} BehaviorExplore;

void         behavior_explore_init(BehaviorExplore *e);
ExploreState behavior_explore_update(BehaviorExplore *e,
                                     const SenseHeatmap *heatmap,
                                     int pet_x, int pet_y,
                                     fix16_t curiosity_level,
                                     double now_sec);

/* Retorna coordenadas para onde o pet deveria se mover (ou -1,-1 se em repouso) */
void behavior_explore_get_target(const BehaviorExplore *e, int *out_x, int *out_y);

/* Atualiza território favorito com base no heatmap */
void behavior_explore_update_home(BehaviorExplore *e, const SenseHeatmap *heatmap);

#endif /* BEHAVIOR_EXPLORE_H */
