#include "softbody.h"
#include <math.h>
#include <stdlib.h>

void softbody_init(SoftBody *sb) {
    sb->scale_x     = 1.0f;
    sb->scale_y     = 1.0f;
    sb->vel_sx      = 0.0f;
    sb->vel_sy      = 0.0f;
    sb->shear       = 0.0f;
    sb->vel_shear   = 0.0f;
    sb->purr_timer  = 0.0f;
    sb->purr_phase  = 0.0f;

    /* Parâmetros equilibrados para física de gelatina macia sem exageros */
    sb->stiffness   = 160.0f;
    sb->damping     = 10.5f;
    sb->mass        = 1.0f;
    sb->restitution = 0.75f;
}

void softbody_squish(SoftBody *sb, float amount) {
    /* Limite seguro para não estourar as bordas do canvas */
    if (amount > 0.18f) amount = 0.18f;
    sb->scale_x += amount;
    sb->vel_sx  += amount * 16.0f;
}

void softbody_stretch(SoftBody *sb, float amount) {
    if (amount > 0.18f) amount = 0.18f;
    sb->scale_x -= amount * 0.70f;
    sb->vel_sx  -= amount * 16.0f;
}

void softbody_jiggle(SoftBody *sb, float intensity) {
    if (intensity > 0.14f) intensity = 0.14f;
    sb->vel_sx    += intensity * 12.0f;
    sb->vel_shear += (float)((rand() % 100) - 50) * 0.01f * intensity * 10.0f;
}

void softbody_purr(SoftBody *sb, float duration) {
    sb->purr_timer = duration;
}

void softbody_set_maturity_stage(SoftBody *sb, int stage) {
    if (!sb) return;
    switch (stage) {
    case 0: /* ONTO_FILHOTE: gelatina macia, altamente responsiva */
        sb->stiffness   = 130.0f;
        sb->damping     = 8.0f;
        sb->mass        = 0.8f;
        sb->restitution = 0.85f;
        break;
    case 1: /* ONTO_JUVENIL */
        sb->stiffness   = 160.0f;
        sb->damping     = 10.5f;
        sb->mass        = 1.0f;
        sb->restitution = 0.75f;
        break;
    case 2: /* ONTO_ADULTO: mais firme e estruturado */
        sb->stiffness   = 200.0f;
        sb->damping     = 13.0f;
        sb->mass        = 1.2f;
        sb->restitution = 0.65f;
        break;
    case 3: /* ONTO_VETERANO: corpo estável, oscilações curtas */
        sb->stiffness   = 240.0f;
        sb->damping     = 16.0f;
        sb->mass        = 1.4f;
        sb->restitution = 0.55f;
        break;
    default:
        break;
    }
}

void softbody_update(SoftBody *sb, double dt, float target_sx, float target_shear) {
    if (dt <= 0.0001) return;
    if (dt > 0.05) dt = 0.05;

    /* Target suavemente limitado */
    if (target_sx < 0.90f) target_sx = 0.90f;
    if (target_sx > 1.12f) target_sx = 1.12f;
    if (target_shear < -0.25f) target_shear = -0.25f;
    if (target_shear >  0.25f) target_shear =  0.25f;

    /* 1. Mola harmônica para scale_x */
    float force_x = -sb->stiffness * (sb->scale_x - target_sx) - sb->damping * sb->vel_sx;
    float accel_x = force_x / sb->mass;
    sb->vel_sx  += accel_x * (float)dt;
    sb->scale_x += sb->vel_sx * (float)dt;

    /* Clamping estrito para garantir que NUNCA saia das bordas */
    if (sb->scale_x < 0.60f) sb->scale_x = 0.60f;
    if (sb->scale_x > 1.02f) sb->scale_x = 1.02f;

    /* Conservação de volume aparente */
    sb->scale_y = 1.0f / sb->scale_x;

    /* 2. Mola harmônica para shear (inclinação sutil) */
    float force_s = -(sb->stiffness * 0.85f) * (sb->shear - target_shear) - (sb->damping * 1.15f) * sb->vel_shear;
    float accel_s = force_s / sb->mass;
    sb->vel_shear += accel_s * (float)dt;
    sb->shear     += sb->vel_shear * (float)dt;

    if (sb->shear < -0.25f) sb->shear = -0.25f;
    if (sb->shear >  0.25f) sb->shear =  0.25f;

    /* 3. Micro-vibração de ronronar (muito suave e agradável) */
    if (sb->purr_timer > 0.0f) {
        sb->purr_timer -= (float)dt;
        sb->purr_phase += (float)dt * 26.0f;
        float purr_amp = 0.012f * (float)sin(sb->purr_phase);
        sb->scale_x += purr_amp;
        sb->scale_y -= purr_amp;
    }
}
