#include "window.h"
#include "window_android.h"
#include "config.h"
#include <android/native_window.h>
#include <android/log.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define LOG_TAG "Caddin"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static struct android_app *g_app = NULL;

void android_set_app_state(struct android_app *app) {
    g_app = app;
}

struct android_app *android_get_app_state(void) {
    return g_app;
}

struct PetWindow {
    struct android_app *app;
    bool should_close;
    bool is_clicked;
    bool was_clicked;
    bool is_dragging;
    int touch_x, touch_y;
    int pos_x, pos_y;
    int width, height;
};

PetWindow *window_create(const char *title, int w, int h) {
    (void)title;
    PetWindow *win = calloc(1, sizeof(PetWindow));
    if (!win) return NULL;
    win->app = g_app;
    win->width = w;
    win->height = h;

    if (g_app && g_app->window) {
        ANativeWindow_setBuffersGeometry(g_app->window, 0, 0, WINDOW_FORMAT_RGBA_8888);
    }

    LOGI("[ANDROID] window_create (%dx%d) pronto", w, h);
    return win;
}

void window_destroy(PetWindow *w) {
    if (w) free(w);
}

bool window_should_close(const PetWindow *w) {
    if (!w || !w->app) return false;
    return w->app->destroyRequested || w->should_close;
}

void window_poll_events(PetWindow *w) {
    if (!w || !w->app) return;
    int events;
    struct android_poll_source *source;
    while (ALooper_pollOnce(0, NULL, &events, (void **)&source) >= 0) {
        if (source != NULL) {
            source->process(w->app, source);
        }
    }
}

void window_swap_buffers(PetWindow *w) {
    (void)w;
}

void window_present(PetWindow *w, const Canvas canvas) {
    if (!w || !w->app || !w->app->window) return;
    ANativeWindow *win = w->app->window;

    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(win, &buffer, NULL) < 0) {
        return;
    }

    if (!buffer.bits || buffer.width <= 0 || buffer.height <= 0 || buffer.stride <= 0) {
        ANativeWindow_unlockAndPost(win);
        return;
    }

    uint32_t *dst = (uint32_t *)buffer.bits;
    int screen_w = buffer.width;
    int screen_h = buffer.height;
    int stride = buffer.stride;

    /* Escala o Caddin de acordo com a largura da tela */
    int scale = screen_w / (CANVAS_W * 1.5f);
    if (scale < 1) scale = 1;
    if (scale > 8) scale = 8;

    int render_w = CANVAS_W * scale;
    int render_h = CANVAS_H * scale;
    int off_x = (screen_w - render_w) / 2;
    int off_y = (screen_h - render_h) / 2;

    /* Limpa a tela com tom escuro suave */
    for (int y = 0; y < screen_h; y++) {
        uint32_t *line = dst + y * stride;
        for (int x = 0; x < screen_w; x++) {
            line[x] = 0xFF181824; /* Dark slate */
        }
    }

    /* Renderiza o sprite do Caddin */
    for (int cy = 0; cy < CANVAS_H; cy++) {
        for (int cx = 0; cx < CANVAS_W; cx++) {
            uint32_t col = canvas[cy * CANVAS_W + cx];
            uint8_t a = (col >> 24) & 0xFF;
            if (a < 10) continue; /* pixel transparente */

            uint8_t r = col & 0xFF;
            uint8_t g = (col >> 8) & 0xFF;
            uint8_t b = (col >> 16) & 0xFF;
            uint32_t native_pixel = (0xFF << 24) | (b << 16) | (g << 8) | r;

            for (int dy = 0; dy < scale; dy++) {
                int py = off_y + cy * scale + dy;
                if (py < 0 || py >= screen_h) continue;
                uint32_t *line = dst + py * stride;
                for (int dx = 0; dx < scale; dx++) {
                    int px = off_x + cx * scale + dx;
                    if (px >= 0 && px < screen_w) {
                        line[px] = native_pixel;
                    }
                }
            }
        }
    }

    ANativeWindow_unlockAndPost(win);
}

char window_read_char(PetWindow *w) { (void)w; return 0; }
void window_update_physics(PetWindow *w, double dt) { (void)w; (void)dt; }
bool window_was_bumped(PetWindow *w) { (void)w; return false; }
bool window_was_clicked(PetWindow *w) {
    if (!w) return false;
    bool c = w->was_clicked;
    w->was_clicked = false;
    return c;
}
bool window_is_dragging(const PetWindow *w) { return w ? w->is_dragging : false; }
double window_get_speed(const PetWindow *w) { (void)w; return 0.0; }
double window_get_vx(const PetWindow *w) { (void)w; return 0.0; }
double window_get_vy(const PetWindow *w) { (void)w; return 0.0; }

void window_get_pos(const PetWindow *w, int *out_x, int *out_y) {
    if (out_x) *out_x = w ? w->pos_x : 0;
    if (out_y) *out_y = w ? w->pos_y : 0;
}

void window_get_workarea(int *out_x, int *out_y, int *out_w, int *out_h) {
    if (out_x) *out_x = 0;
    if (out_y) *out_y = 0;
    if (out_w) *out_w = 1080;
    if (out_h) *out_h = 2400;
}

void window_apply_impulse(PetWindow *w, double fx, double fy) { (void)w; (void)fx; (void)fy; }
int window_get_wheel(PetWindow *w) { (void)w; return 0; }
bool window_get_active_window_rect(PetWindow *w, int *out_x, int *out_y, int *out_ww, int *out_hh, uint64_t *out_id) {
    (void)w;
    if (out_x) *out_x = 0;
    if (out_y) *out_y = 0;
    if (out_ww) *out_ww = 0;
    if (out_hh) *out_hh = 0;
    if (out_id) *out_id = 0;
    return false;
}
