/* sense_luma.c — amostragem de luminosidade sob o pet via XGetImage (Linux)
 * Windows: GDI BitBlt + GetPixel.
 * Fallback: retorna 0.5 (neutro) se não conseguir capturar.
 */
#define _POSIX_C_SOURCE 200809L
#include "sense_luma.h"
#include <stdlib.h>
#include <string.h>

#if defined(__linux__) && !defined(_WIN32) && !defined(CADDIN_HEADLESS)

#include <X11/Xlib.h>
#include <X11/Xutil.h>

struct SenseLuma {
    Display *dpy;
    Window   root;
    fix16_t  last_luma;
};

SenseLuma *sense_luma_create(void *display_ptr, unsigned long root_window) {
    SenseLuma *s = calloc(1, sizeof(SenseLuma));
    if (!s) return NULL;
    s->dpy       = (Display *)display_ptr;
    s->root      = (Window)root_window;
    s->last_luma = fix_from_float(0.5f);
    return s;
}

void sense_luma_destroy(SenseLuma *s) {
    free(s);
}

fix16_t sense_luma_sample(SenseLuma *s, int cx, int cy, int side) {
    if (!s || !s->dpy) return s ? s->last_luma : fix_from_float(0.5f);

    int x = cx - side / 2;
    int y = cy - side / 2;
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    XImage *img = XGetImage(s->dpy, s->root, x, y, (unsigned)side, (unsigned)side,
                            AllPlanes, ZPixmap);
    if (!img) return s->last_luma;

    long long sum = 0;
    int pixels = side * side;
    for (int py = 0; py < side; py++) {
        for (int px2 = 0; px2 < side; px2++) {
            unsigned long pix = XGetPixel(img, px2, py);
            int r = (int)((pix >> 16) & 0xFF);
            int g = (int)((pix >>  8) & 0xFF);
            int b = (int)( pix        & 0xFF);
            /* Luma BT.709 */
            sum += (2126 * r + 7152 * g + 722 * b);
        }
    }
    XDestroyImage(img);

    /* sum / pixels / 10000 → 0..1 */
    float luma = (float)(sum / pixels) / (10000.0f * 255.0f);
    if (luma < 0.0f) luma = 0.0f;
    if (luma > 1.0f) luma = 1.0f;
    s->last_luma = fix_from_float(luma);
    return s->last_luma;
}

#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct SenseLuma {
    fix16_t last_luma;
};

SenseLuma *sense_luma_create(void *display_ptr, unsigned long root_window) {
    (void)display_ptr; (void)root_window;
    SenseLuma *s = calloc(1, sizeof(SenseLuma));
    if (!s) return NULL;
    s->last_luma = fix_from_float(0.5f);
    return s;
}

void sense_luma_destroy(SenseLuma *s) { free(s); }

fix16_t sense_luma_sample(SenseLuma *s, int cx, int cy, int side) {
    if (!s) return fix_from_float(0.5f);
    HDC hdc_screen = GetDC(NULL);
    if (!hdc_screen) return s->last_luma;

    HDC hdc_mem = CreateCompatibleDC(hdc_screen);
    HBITMAP hbm = CreateCompatibleBitmap(hdc_screen, side, side);
    SelectObject(hdc_mem, hbm);
    int x = cx - side / 2, y = cy - side / 2;
    BitBlt(hdc_mem, 0, 0, side, side, hdc_screen, x, y, SRCCOPY);

    long long sum = 0;
    for (int py = 0; py < side; py++) {
        for (int px2 = 0; px2 < side; px2++) {
            COLORREF c = GetPixel(hdc_mem, px2, py);
            int r = GetRValue(c), g = GetGValue(c), b = GetBValue(c);
            sum += 2126 * r + 7152 * g + 722 * b;
        }
    }

    DeleteObject(hbm);
    DeleteDC(hdc_mem);
    ReleaseDC(NULL, hdc_screen);

    int pixels = side * side;
    float luma = (float)(sum / pixels) / (10000.0f * 255.0f);
    if (luma < 0.0f) luma = 0.0f;
    if (luma > 1.0f) luma = 1.0f;
    s->last_luma = fix_from_float(luma);
    return s->last_luma;
}

#else

struct SenseLuma { fix16_t last_luma; };
SenseLuma *sense_luma_create(void *d, unsigned long r) {
    (void)d; (void)r;
    SenseLuma *s = calloc(1, sizeof(SenseLuma));
    if (s) s->last_luma = fix_from_float(0.5f);
    return s;
}
void    sense_luma_destroy(SenseLuma *s)                   { free(s); }
fix16_t sense_luma_sample(SenseLuma *s, int cx, int cy, int side) {
    (void)cx; (void)cy; (void)side;
    return s ? s->last_luma : fix_from_float(0.5f);
}

#endif
