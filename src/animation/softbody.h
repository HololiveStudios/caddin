#ifndef CORTEX_SOFTBODY_H
#define CORTEX_SOFTBODY_H

#include <stdbool.h>

/*
 * Simulação de física soft-body ("jelly" / "squishy") com massa-mola e conservação de volume.
 */
typedef struct {
    /* Estado elástico */
    float scale_x;      /* Deformação horizontal (1.0 = repouso) */
    float scale_y;      /* Deformação vertical (1.0 = repouso, conserva volume) */
    float vel_sx;       /* Velocidade da mola em X */
    float vel_sy;       /* Velocidade da mola em Y */

    float shear;        /* Inclinação/inércia angular */
    float vel_shear;

    /* Purr (ronronar / micro-vibração em afeto) */
    float purr_timer;
    float purr_phase;

    /* Parâmetros físicos ajustáveis */
    float stiffness;    /* Rigidez elástica da gelatina */
    float damping;      /* Amortecimento da oscilação */
    float mass;         /* Massa */
    float restitution;  /* Elasticidade de impacto */
} SoftBody;

void  softbody_init     (SoftBody *sb);
void  softbody_update   (SoftBody *sb, double dt, float target_sx, float target_shear);
void  softbody_squish   (SoftBody *sb, float amount);   /* Achatamento no chão / impacto */
void  softbody_stretch  (SoftBody *sb, float amount);   /* Esticamento vertical / salto */
void  softbody_jiggle   (SoftBody *sb, float intensity);/* Cutucada / clique / pulo */
void  softbody_purr     (SoftBody *sb, float duration); /* Ronronar em carinho */

#endif /* CORTEX_SOFTBODY_H */
