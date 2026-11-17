#ifndef CORTEX_FACE_H
#define CORTEX_FACE_H

#include "pixel_draw.h"
#include "softbody.h"
#include "emotions.h"

typedef enum {
    MOUTH_NEUTRAL = 0,
    MOUTH_SMILE,
    MOUTH_FROWN,
    MOUTH_OPEN,
    MOUTH_WORRIED,
    MOUTH_WAVY
} MouthShape;

typedef struct {
    float openness;   /* 0=fechado, 1=normal, 1.2=arregalado */
    float pupil_x;    /* -1..+1, offset horizontal da pupila */
    float pupil_y;    /* -1..+1, offset vertical da pupila */
    float expr_lid;   /* 0=aberto, 1=fechado (expressão: triste/sonolento) */
} EyeState;

typedef struct {
    MouthShape shape;
    float openness;   /* 0..1, usado em MOUTH_OPEN */
    float width;      /* 0..1, multiplica largura base */
    float curvature;  /* 0..1, profundidade do arco */
} MouthState;

typedef struct {
    EyeState  left_eye;
    EyeState  right_eye;
    MouthState mouth;
    float     tilt;
} FaceState;

/* Desenho clássico */
void face_draw(Canvas buf, const FaceState *face, int bob_y, float blink_lid);

/* Desenho com física soft-body e emoções reativas */
void face_draw_squishy(Canvas buf, const FaceState *face,
                       int bob_y, float blink_lid,
                       const SoftBody *sb, const EmotionMachine *em);

void face_default(FaceState *face);

#endif /* CORTEX_FACE_H */
