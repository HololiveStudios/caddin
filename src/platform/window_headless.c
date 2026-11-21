/*
 * window_headless.c — stub sem display para compilação Android/ARM64/CLI
 *
 * Implementa toda a interface de window.h com operações no-op.
 * Permite compilar e executar o núcleo cognitivo (SNN, ontogenia, SDM,
 * bandit, biometria, telemetria) sem dependência de X11 ou Win32.
 *
 * Uso: compile com -DCADDIN_HEADLESS
 */
#include "window.h"
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

struct PetWindow {
    bool  should_close;
    int   width, height;
    char  pending_key;
};

PetWindow *window_create(const char *title, int w, int h)
{
    PetWindow *win = calloc(1, sizeof(PetWindow));
    if (!win) return NULL;
    win->width  = w;
    win->height = h;
    fprintf(stderr, "[HEADLESS] window_create: %s (%dx%d) — sem display\n",
            title ? title : "caddin", w, h);
    return win;
}

void window_destroy(PetWindow *w)
{
    if (w) free(w);
}

bool window_should_close(const PetWindow *w)
{
    return w ? w->should_close : true;
}

void window_poll_events(PetWindow *w)
{
    /* sem eventos de display — loop termina via SIGINT / sinal */
    (void)w;
}

void window_swap_buffers(PetWindow *w)
{
    (void)w;
}

void window_present(PetWindow *w, const Canvas canvas)
{
    (void)w;
    (void)canvas;
}

char window_read_char(PetWindow *w)
{
    if (!w) return 0;
    char c = w->pending_key;
    w->pending_key = 0;
    return c;
}

void window_update_physics(PetWindow *w, double dt)
{
    (void)w;
    (void)dt;
}

bool window_was_bumped(PetWindow *w)   { (void)w; return false; }
bool window_was_clicked(PetWindow *w)  { (void)w; return false; }
bool window_is_dragging(const PetWindow *w) { (void)w; return false; }

double window_get_speed(const PetWindow *w) { (void)w; return 0.0; }
double window_get_vx(const PetWindow *w)    { (void)w; return 0.0; }
double window_get_vy(const PetWindow *w)    { (void)w; return 0.0; }

void window_get_pos(const PetWindow *w, int *out_x, int *out_y)
{
    (void)w;
    if (out_x) *out_x = 0;
    if (out_y) *out_y = 0;
}

void window_get_workarea(int *out_x, int *out_y, int *out_w, int *out_h)
{
    if (out_x) *out_x = 0;
    if (out_y) *out_y = 0;
    if (out_w) *out_w = 1920;
    if (out_h) *out_h = 1080;
}

void window_apply_impulse(PetWindow *w, double fx, double fy)
{
    (void)w; (void)fx; (void)fy;
}

int window_get_wheel(PetWindow *w)
{
    (void)w;
    return 0;
}

bool window_get_active_window_rect(PetWindow *w,
                                   int *out_x, int *out_y,
                                   int *out_ww, int *out_hh,
                                   uint64_t *out_id)
{
    (void)w;
    if (out_x)  *out_x  = 0;
    if (out_y)  *out_y  = 0;
    if (out_ww) *out_ww = 0;
    if (out_hh) *out_hh = 0;
    if (out_id) *out_id = 0;
    return false;
}
