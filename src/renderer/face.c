#include "face.h"
#include <stdbool.h>
#include <math.h>
#include <string.h>

/* ── Constantes base de layout para 4:3 (Canvas 128×96) com margens seguras ── */
#define BODY_W_BASE    96.0f
#define BODY_H_BASE    72.0f
#define BODY_R_BASE    13.0f

#define SCREEN_W_BASE  82.0f
#define SCREEN_H_BASE  58.0f
#define SCREEN_R_BASE   9.0f

#define EYE_HALF_W_BASE 7.0f
#define EYE_HALF_H_BASE 11.5f
#define EYE_R_BASE      5.0f

static void draw_cyber_eye(Canvas buf, int cx, int cy,
                           float scale_x, float scale_y,
                           const EyeState *e, float blink_lid,
                           bool is_left, PetEmotion emo, const MouthState *mouth) {

    /* ── Estado Especial: Tontura (EMO_DIZZY) → Olhos em "X" ── */
    if (emo == EMO_DIZZY) {
        int r = (int)roundf(7.0f * (scale_x < scale_y ? scale_x : scale_y));
        if (r < 4) r = 4;
        for (int d = -r; d <= r; d++) {
            pd_set(buf, cx + d, cy + d, COL_NEON_CORE);
            pd_set(buf, cx + d, cy + d - 1, COL_NEON_GLOW);
            pd_set(buf, cx + d, cy - d, COL_NEON_CORE);
            pd_set(buf, cx + d, cy - d - 1, COL_NEON_GLOW);
        }
        return;
    }

    float open = e->openness * (1.0f - blink_lid);
    if (open < 0.f) open = 0.f;

    /* ── Estado: Piscar / Fechado (linhas finas '--  --') ── */
    if (open < 0.18f) {
        int w = (int)roundf(15.0f * scale_x);
        int h = 3;
        int x0 = cx - w / 2;
        int y0 = cy;
        for (int r = y0; r < y0 + h; r++) {
            for (int c = x0; c < x0 + w; c++) {
                bool end = (c == x0 || c == x0 + w - 1);
                pd_set(buf, c, r, end ? COL_NEON_GLOW : COL_NEON_CORE);
            }
        }
        return;
    }

    /* ── Estado: Carinho / Feliz (arcos curvados '^  ^') ── */
    bool is_happy = (emo == EMO_AFFECTION) ||
                    (mouth->shape == MOUTH_SMILE && mouth->curvature > 0.40f);
    if (is_happy && open > 0.60f) {
        int hw = (int)roundf(9.0f * scale_x);
        int peak = (int)roundf(5.0f * scale_y);
        for (int dx = -hw; dx <= hw; dx++) {
            float t = (float)dx / (float)hw;
            int yi = cy - (int)roundf((float)peak * (1.f - t*t)) + 2;
            for (int th = 0; th < 3; th++) {
                pd_set(buf, cx + dx, yi + th, th == 0 || th == 2 ? COL_NEON_GLOW : COL_NEON_CORE);
            }
        }
        return;
    }

    /* ── Estado: Cápsulas verticais expressivas ── */
    int cur_hw = (int)roundf(EYE_HALF_W_BASE * scale_x);
    if (cur_hw < 4) cur_hw = 4;

    float open_mult = (emo == EMO_SURPRISED) ? 1.30f : (open > 1.25f ? 1.25f : open);
    int cur_hh = (int)roundf(EYE_HALF_H_BASE * scale_y * open_mult);
    if (cur_hh < 4) cur_hh = 4;

    /* Desvio de olhar */
    int ox = (int)roundf(e->pupil_x * 4.0f * scale_x);
    int oy = (int)roundf(e->pupil_y * 3.0f * scale_y);
    int ecx = cx + ox;
    int ecy = cy + oy;

    int x0 = ecx - cur_hw;
    int x1 = ecx + cur_hw;
    int y0 = ecy - cur_hh;
    int y1 = ecy + cur_hh;
    int eye_r = (int)roundf(EYE_R_BASE * (scale_x < scale_y ? scale_x : scale_y));
    if (eye_r < 3) eye_r = 3;

    bool is_angry = (emo == EMO_ANNOYED) || (mouth->shape == MOUTH_WORRIED && e->expr_lid > 0.20f);
    bool is_sad   = (mouth->shape == MOUTH_FROWN && e->expr_lid > 0.20f);
    float lid_cut = e->expr_lid > blink_lid ? e->expr_lid : blink_lid;

    for (int row = y0; row <= y1; row++) {
        for (int col = x0; col <= x1; col++) {
            /* 1. Corte de cápsula */
            int cl = col < x0 + eye_r, cr = col > x1 - eye_r;
            int ct = row < y0 + eye_r, cb = row > y1 - eye_r;
            if (cl && ct) { int dx = col - (x0 + eye_r), dy = row - (y0 + eye_r); if (dx*dx + dy*dy > eye_r*eye_r) continue; }
            if (cr && ct) { int dx = col - (x1 - eye_r), dy = row - (y0 + eye_r); if (dx*dx + dy*dy > eye_r*eye_r) continue; }
            if (cl && cb) { int dx = col - (x0 + eye_r), dy = row - (y1 - eye_r); if (dx*dx + dy*dy > eye_r*eye_r) continue; }
            if (cr && cb) { int dx = col - (x1 - eye_r), dy = row - (y1 - eye_r); if (dx*dx + dy*dy > eye_r*eye_r) continue; }

            /* 2. Corte diagonal para bravo (\  /) */
            if (is_angry) {
                float norm_x = (float)(col - ecx) / (float)cur_hw;
                float cut_y = (float)y0 + (is_left ? (norm_x + 1.f) * 6.f : (1.f - norm_x) * 6.f);
                if ((float)row < cut_y) continue;
            }
            /* 3. Corte diagonal para triste (/  \) */
            else if (is_sad) {
                float norm_x = (float)(col - ecx) / (float)cur_hw;
                float cut_y = (float)y0 + (is_left ? (1.f - norm_x) * 5.f : (norm_x + 1.f) * 5.f);
                if ((float)row < cut_y) continue;
            }
            /* 4. Corte de pálpebra superior (sonolento) */
            else if (lid_cut > 0.15f) {
                int cut_h = (int)((float)(y1 - y0) * lid_cut * 0.65f);
                if (row < y0 + cut_h) continue;
            }

            bool border = (col == x0 || col == x1 || row == y0 || row == y1);
            pd_set(buf, col, row, border ? COL_NEON_GLOW : COL_NEON_CORE);
        }
    }

    /* ── Ponto de reflexo especular branco (Glint dos olhos) ── */
    if (open > 0.35f && !is_angry) {
        int gx = is_left ? (ecx + (int)(2.f * scale_x)) : (ecx - (int)(4.f * scale_x));
        int gy = y0 + 3;
        pd_fill_rect(buf, gx, gy, 2, 2, COL_NEON_WHITE);
        pd_set(buf, gx + 2, gy + 1, COL_NEON_CORE);
    }
}

static void draw_cyber_mouth(Canvas buf, int cx, int cy, float scale_x, float scale_y,
                            const MouthState *m, PetEmotion emo) {
    int hw = (int)roundf((6.f + m->width * 6.f) * scale_x);
    if (hw < 2) hw = 2;

    /* Boca ondulada em tontura */
    if (emo == EMO_DIZZY) {
        for (int dx = -hw; dx <= hw; dx++) {
            int dy = (int)roundf(2.0f * sinf((float)dx * 0.7f));
            pd_set(buf, cx + dx, cy + dy, COL_NEON_CORE);
            pd_set(buf, cx + dx, cy + dy + 1, COL_NEON_GLOW);
        }
        return;
    }

    /* Boca aberta de susto */
    if (emo == EMO_SURPRISED) {
        int eh = (int)roundf(7.0f * scale_y);
        if (eh < 3) eh = 3;
        pd_fill_ellipse(buf, cx, cy + eh/2, hw - 1, eh, COL_NEON_CORE);
        return;
    }

    switch (m->shape) {
    case MOUTH_NEUTRAL:
        pd_fill_rect(buf, cx - (int)(4.f * scale_x), cy, (int)(8.f * scale_x), 2, COL_NEON_CORE);
        break;

    case MOUTH_SMILE: {
        int peak = (int)roundf(m->curvature * 5.f * scale_y);
        if (peak < 1) peak = 1;
        pd_parabola_arc(buf, cx, cy, hw, peak, 2, COL_NEON_CORE);
        break;
    }

    case MOUTH_FROWN: {
        int peak = -(int)roundf(m->curvature * 5.f * scale_y);
        if (peak > -1) peak = -1;
        pd_parabola_arc(buf, cx, cy, hw, peak, 2, COL_NEON_CORE);
        break;
    }

    case MOUTH_OPEN: {
        int eh = (int)roundf(m->openness * 7.f * scale_y);
        if (eh < 2) eh = 2;
        pd_fill_ellipse(buf, cx, cy + eh/2, hw - 1, eh, COL_NEON_CORE);
        break;
    }

    case MOUTH_WORRIED:
    case MOUTH_WAVY:
        pd_fill_rect(buf, cx - 4, cy, 8, 2, COL_NEON_CORE);
        pd_set(buf, cx - 5, cy + 1, COL_NEON_GLOW);
        pd_set(buf, cx + 4, cy - 1, COL_NEON_GLOW);
        break;
    }
}

/* ── API pública ── */

void face_default(FaceState *f) {
    memset(f, 0, sizeof(*f));
    f->left_eye.openness  = 1.f;
    f->right_eye.openness = 1.f;
    f->mouth.shape        = MOUTH_NEUTRAL;
    f->mouth.width        = 0.4f;
    f->mouth.curvature    = 0.5f;
}

void face_draw(Canvas buf, const FaceState *face, int bob_y, float blink_lid) {
    SoftBody sb;
    softbody_init(&sb);
    EmotionMachine em;
    emot_init(&em);
    face_draw_squishy(buf, face, bob_y, blink_lid, &sb, &em);
}

void face_draw_squishy(Canvas buf, const FaceState *face,
                       int bob_y, float blink_lid,
                       const SoftBody *sb, const EmotionMachine *em) {
    pd_clear(buf, COL_TRANSPARENT);

    int cx = CANVAS_W / 2;
    int cy = (CANVAS_H / 2) + bob_y;

    float sx = sb->scale_x * em->breath_scale;
    float sy = sb->scale_y * em->breath_scale;

    /* 1. Corpo gelatinoso 3D com luz, sombra e deformação soft-body */
    int bw = (int)roundf(BODY_W_BASE * sx);
    int bh = (int)roundf(BODY_H_BASE * sy);
    int bx = cx - bw / 2;
    int by = cy - bh / 2;
    int br = (int)roundf(BODY_R_BASE * (sx < sy ? sx : sy));
    if (br < 5) br = 5;

    pd_shaded_body(buf, bx, by, bw, bh, br);

    /* 2. Visor escuro de vidro cibernético deformado */
    int sw = (int)roundf(SCREEN_W_BASE * sx);
    int sh = (int)roundf(SCREEN_H_BASE * sy);
    int sx_pos = cx - sw / 2;
    int sy_pos = cy - sh / 2;
    int sr = (int)roundf(SCREEN_R_BASE * (sx < sy ? sx : sy));
    if (sr < 4) sr = 4;

    pd_draw_screen(buf, sx_pos, sy_pos, sw, sh, sr);

    /* 3. Olhos cibernéticos estilo robô com emoções reativas */
    int eye_spacing = (int)roundf(17.0f * sx);
    int eye_lx = cx - eye_spacing;
    int eye_rx = cx + eye_spacing;
    int eye_y  = cy - (int)roundf(4.0f * sy);

    float lid = em->blinking ? em->blink_lid : blink_lid;

    EyeState left_copy = face->left_eye;
    EyeState right_copy = face->right_eye;

    /* No modo curioso ou idle, olha suavemente para onde o usuário está */
    if (em->current == EMO_IDLE || em->current == EMO_CURIOUS) {
        left_copy.pupil_x  += em->look_x;
        left_copy.pupil_y  += em->look_y;
        right_copy.pupil_x += em->look_x;
        right_copy.pupil_y += em->look_y;
    }

    draw_cyber_eye(buf, eye_lx, eye_y, sx, sy,
                   &left_copy, lid, true, em->current, &face->mouth);
    draw_cyber_eye(buf, eye_rx, eye_y, sx, sy,
                   &right_copy, lid, false, em->current, &face->mouth);

    /* 4. Boca neon reativa */
    int mouth_y = cy + (int)roundf(16.0f * sy);
    draw_cyber_mouth(buf, cx, mouth_y, sx, sy, &face->mouth, em->current);
}
