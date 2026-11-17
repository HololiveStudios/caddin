#include "pixel_draw.h"
#include <math.h>
#include <string.h>

void pd_clear(Canvas buf, Pixel c) {
    for (int i = 0; i < CANVAS_W * CANVAS_H; i++) buf[i] = c;
}

static inline int in_bounds(int x, int y) {
    return (unsigned)x < (unsigned)CANVAS_W &&
           (unsigned)y < (unsigned)CANVAS_H;
}

void pd_set(Canvas buf, int x, int y, Pixel c) {
    if (in_bounds(x, y)) buf[y * CANVAS_W + x] = c;
}

void pd_fill_rect(Canvas buf, int x, int y, int w, int h, Pixel c) {
    for (int r = y; r < y + h; r++)
        for (int col = x; col < x + w; col++)
            pd_set(buf, col, r, c);
}

void pd_fill_circle(Canvas buf, int cx, int cy, int r, Pixel c) {
    for (int dy = -r; dy <= r; dy++)
        for (int dx = -r; dx <= r; dx++)
            if (dx*dx + dy*dy <= r*r)
                pd_set(buf, cx+dx, cy+dy, c);
}

void pd_fill_ellipse(Canvas buf, int cx, int cy, int rx, int ry, Pixel c) {
    if (rx <= 0 || ry <= 0) return;
    float irx2 = 1.f / (float)(rx * rx);
    float iry2 = 1.f / (float)(ry * ry);
    for (int dy = -ry; dy <= ry; dy++)
        for (int dx = -rx; dx <= rx; dx++)
            if ((float)(dx*dx)*irx2 + (float)(dy*dy)*iry2 <= 1.f)
                pd_set(buf, cx+dx, cy+dy, c);
}

static int inside_rrect(int col, int row,
                         int x0, int y0, int x1, int y1, int r) {
    if (col < x0 || col > x1 || row < y0 || row > y1) return 0;
    /* checar cantos */
    int cl = col < x0+r, cr = col > x1-r;
    int ct = row < y0+r, cb = row > y1-r;
    if (cl && ct) { int dx=col-(x0+r), dy=row-(y0+r); return dx*dx+dy*dy<=r*r; }
    if (cr && ct) { int dx=col-(x1-r), dy=row-(y0+r); return dx*dx+dy*dy<=r*r; }
    if (cl && cb) { int dx=col-(x0+r), dy=row-(y1-r); return dx*dx+dy*dy<=r*r; }
    if (cr && cb) { int dx=col-(x1-r), dy=row-(y1-r); return dx*dx+dy*dy<=r*r; }
    return 1;
}

void pd_rounded_rect(Canvas buf, int x, int y, int w, int h, int r,
                     Pixel fill, Pixel border) {
    int x1 = x+w-1, y1 = y+h-1;
    for (int row = y; row <= y1; row++) {
        for (int col = x; col <= x1; col++) {
            if (!inside_rrect(col, row, x, y, x1, y1, r)) continue;
            /* borda: algum vizinho fora do rrect */
            int is_border =
                !inside_rrect(col-1, row,   x, y, x1, y1, r) ||
                !inside_rrect(col+1, row,   x, y, x1, y1, r) ||
                !inside_rrect(col,   row-1, x, y, x1, y1, r) ||
                !inside_rrect(col,   row+1, x, y, x1, y1, r);
            pd_set(buf, col, row, is_border ? border : fill);
        }
    }
}

void pd_parabola_arc(Canvas buf, int cx, int cy, int hw,
                     int peak, int thickness, Pixel c) {
    if (hw <= 0) return;
    for (int dx = -hw; dx <= hw; dx++) {
        float t  = (float)dx / (float)hw;
        float yf = (float)cy + (float)peak * (1.f - t*t);
        int   yi = (int)roundf(yf);
        for (int th = 0; th < thickness; th++)
            pd_set(buf, cx+dx, yi+th, c);
    }
}

void pd_shaded_body(Canvas buf, int x, int y, int w, int h, int r) {
    int x1 = x + w - 1;
    int y1 = y + h - 1;
    float cx = (float)x + (float)w * 0.5f;
    float cy = (float)y + (float)h * 0.5f;
    float hw = (float)w * 0.5f;
    float hh = (float)h * 0.5f;

    /* Vetor de luz vindo do topo-esquerda-frontal */
    const float lx = -0.4082f, ly = -0.5773f, lz = 0.7071f;
    /* Half-vector para reflexo especular */
    const float hx = -0.231f, hy = -0.327f, hz = 0.916f;

    const float bevel = 16.0f;

    for (int row = y; row <= y1; row++) {
        for (int col = x; col <= x1; col++) {
            if (!inside_rrect(col, row, x, y, x1, y1, r)) continue;

            int is_border =
                !inside_rrect(col-1, row,   x, y, x1, y1, r) ||
                !inside_rrect(col+1, row,   x, y, x1, y1, r) ||
                !inside_rrect(col,   row-1, x, y, x1, y1, r) ||
                !inside_rrect(col,   row+1, x, y, x1, y1, r);

            float dcx = (float)col - cx;
            float dcy = (float)row - cy;

            float d;
            float out_x = 0.f, out_y = 0.f;

            int cl = col < x + r, cr = col > x1 - r;
            int ct = row < y + r, cb = row > y1 - r;

            if (cl && ct) {
                float ox = (float)(col - (x + r));
                float oy = (float)(row - (y + r));
                float dist = sqrtf(ox * ox + oy * oy);
                d = (float)r - dist;
                if (dist > 0.001f) { out_x = ox / dist; out_y = oy / dist; }
            } else if (cr && ct) {
                float ox = (float)(col - (x1 - r));
                float oy = (float)(row - (y + r));
                float dist = sqrtf(ox * ox + oy * oy);
                d = (float)r - dist;
                if (dist > 0.001f) { out_x = ox / dist; out_y = oy / dist; }
            } else if (cl && cb) {
                float ox = (float)(col - (x + r));
                float oy = (float)(row - (y1 - r));
                float dist = sqrtf(ox * ox + oy * oy);
                d = (float)r - dist;
                if (dist > 0.001f) { out_x = ox / dist; out_y = oy / dist; }
            } else if (cr && cb) {
                float ox = (float)(col - (x1 - r));
                float oy = (float)(row - (y1 - r));
                float dist = sqrtf(ox * ox + oy * oy);
                d = (float)r - dist;
                if (dist > 0.001f) { out_x = ox / dist; out_y = oy / dist; }
            } else {
                float dl = (float)(col - x);
                float dr = (float)(x1 - col);
                float dt_edge = (float)(row - y);
                float db = (float)(y1 - row);
                d = dl; out_x = -1.f; out_y = 0.f;
                if (dr < d) { d = dr; out_x = 1.f; out_y = 0.f; }
                if (dt_edge < d) { d = dt_edge; out_x = 0.f; out_y = -1.f; }
                if (db < d) { d = db; out_x = 0.f; out_y = 1.f; }
            }

            if (is_border) {
                /* Borda sombreada: clara no topo-esq, mais escura embaixo */
                float b_dot = -(out_x * lx + out_y * ly);
                float bt = (b_dot + 1.0f) * 0.5f;
                int br = (int)(160.f + bt * 75.f);
                int bg = (int)(170.f + bt * 72.f);
                int bb = (int)(190.f + bt * 62.f);
                pd_set(buf, col, row, PX(br, bg, bb, 255));
                continue;
            }

            /* Cálculo da normal volumétrica 3D */
            float nx, ny, nz;
            if (d < bevel) {
                float t = 1.0f - d / bevel;
                float sin_th = t;
                float cos_th = sqrtf(fmaxf(0.f, 1.f - sin_th * sin_th));
                nx = out_x * sin_th;
                ny = out_y * sin_th;
                nz = cos_th;
            } else {
                /* Curvatura convexa frontal suave */
                nx = 0.22f * (dcx / hw);
                ny = 0.22f * (dcy / hh);
                nz = sqrtf(fmaxf(0.01f, 1.0f - nx * nx - ny * ny));
            }
            float nlen = sqrtf(nx * nx + ny * ny + nz * nz);
            if (nlen > 0.001f) { nx /= nlen; ny /= nlen; nz /= nlen; }

            /* Iluminação Difusa */
            float diff = nx * lx + ny * ly + nz * lz;
            if (diff < 0.f) diff = 0.f;

            /* Gradiente vertical de ambient light (topo mais iluminado) */
            float vert = 1.0f - 0.18f * ((float)(row - y) / (float)h);

            /* Brilho especular 3D (sheen) */
            float s_dot = nx * hx + ny * hy + nz * hz;
            float spec = 0.f;
            if (s_dot > 0.f) {
                spec = s_dot * s_dot;
                spec = spec * spec;
                spec = spec * spec;
                spec = spec * spec;
                spec *= 0.60f;
            }

            /* Paleta: sombra perolada (205, 214, 230) até luz pura (255, 255, 255) */
            float light = (0.28f + 0.72f * diff) * vert;
            float r_val = 205.f + light * 50.f + spec * 255.f;
            float g_val = 214.f + light * 41.f + spec * 255.f;
            float b_val = 230.f + light * 25.f + spec * 255.f;

            /* Bochechas suaves rosadas */
            float b1x = cx - 28.f, b1y = cy + 16.f;
            float b2x = cx + 28.f, b2y = cy + 16.f;
            float d_b1 = sqrtf((col - b1x)*(col - b1x) + (row - b1y)*(row - b1y));
            float d_b2 = sqrtf((col - b2x)*(col - b2x) + (row - b2y)*(row - b2y));
            float blush_dist = d_b1 < d_b2 ? d_b1 : d_b2;
            if (blush_dist < 8.5f) {
                float b_alpha = (1.f - blush_dist / 8.5f) * 0.28f;
                r_val = r_val * (1.f - b_alpha) + 255.f * b_alpha;
                g_val = g_val * (1.f - b_alpha) + 145.f * b_alpha;
                b_val = b_val * (1.f - b_alpha) + 170.f * b_alpha;
            }

            int ir = (int)fminf(255.f, fmaxf(0.f, r_val));
            int ig = (int)fminf(255.f, fmaxf(0.f, g_val));
            int ib = (int)fminf(255.f, fmaxf(0.f, b_val));

            pd_set(buf, col, row, PX(ir, ig, ib, 255));
        }
    }
}

void pd_draw_screen(Canvas buf, int x, int y, int w, int h, int r) {
    int x1 = x + w - 1;
    int y1 = y + h - 1;

    for (int row = y; row <= y1; row++) {
        for (int col = x; col <= x1; col++) {
            if (!inside_rrect(col, row, x, y, x1, y1, r)) continue;

            int is_border =
                !inside_rrect(col-1, row,   x, y, x1, y1, r) ||
                !inside_rrect(col+1, row,   x, y, x1, y1, r) ||
                !inside_rrect(col,   row-1, x, y, x1, y1, r) ||
                !inside_rrect(col,   row+1, x, y, x1, y1, r);

            if (is_border) {
                pd_set(buf, col, row, COL_CYBER_BEZEL);
                continue;
            }

            /* Fundo escuro do visor com leve reflexo de vidro no topo */
            float v = (float)(row - y) / (float)h;
            int r_c = 15, g_c = 19, b_c = 24;
            if (v < 0.40f) {
                float glass = (1.0f - v / 0.40f) * 16.f;
                r_c += (int)glass;
                g_c += (int)(glass * 1.2f);
                b_c += (int)(glass * 1.5f);
            }
            pd_set(buf, col, row, PX(r_c, g_c, b_c, 255));
        }
    }
}
