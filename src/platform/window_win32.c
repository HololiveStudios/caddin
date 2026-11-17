#include "window.h"
#include "config.h"

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define KEY_BUF 64

struct PetWindow {
    HWND hwnd;
    HDC  hdcMem;
    HBITMAP hbmp;
    HBITMAP hbmpOld;
    uint32_t *pixels;

    int width, height;

    /* drag e clique */
    bool   mouse_down;
    bool   dragging;
    bool   is_click;
    bool   was_clicked;
    double press_time;
    int    press_x, press_y;
    int    grab_ox, grab_oy;

    /* física de corpo rígido da janela */
    double pos_x, pos_y;
    double vx, vy;
    bool   in_physics;
    bool   bumped;

    /* fechamento */
    bool should_close;

    /* fila circular de teclas */
    char  key_buf[KEY_BUF];
    int   key_head, key_tail;

    /* scroll wheel */
    int   wheel_delta;
};

static struct PetWindow *g_active_win = NULL;

static double now_sec(void) {
    static LARGE_INTEGER freq;
    static int initialized = 0;
    if (!initialized) {
        QueryPerformanceFrequency(&freq);
        initialized = 1;
    }
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return (double)count.QuadPart / (double)freq.QuadPart;
}

static LRESULT CALLBACK pet_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PetWindow *w = (PetWindow *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_LBUTTONDOWN:
        if (w) {
            SetCapture(hwnd);
            w->mouse_down = true;
            w->dragging   = false;
            w->is_click   = true;
            w->press_time = now_sec();
            w->in_physics = false;
            w->vx = 0.0;
            w->vy = 0.0;

            POINT pt;
            GetCursorPos(&pt);
            w->press_x = pt.x;
            w->press_y = pt.y;
            w->grab_ox = pt.x - (int)round(w->pos_x);
            w->grab_oy = pt.y - (int)round(w->pos_y);
        }
        return 0;

    case WM_LBUTTONUP:
        if (w) {
            ReleaseCapture();
            double dt_click = now_sec() - w->press_time;
            if (w->is_click && dt_click < 0.28) {
                w->was_clicked = true;
            }
            w->mouse_down = false;
            w->dragging   = false;

            double speed = hypot(w->vx, w->vy);
            if (speed > 50.0) {
                w->in_physics = true;
                if (speed > 3000.0) {
                    double s = 3000.0 / speed;
                    w->vx *= s;
                    w->vy *= s;
                }
            } else {
                w->vx = 0.0;
                w->vy = 0.0;
                w->in_physics = false;
            }
        }
        return 0;

    case WM_KEYDOWN:
        if (w) {
            if (wParam == VK_ESCAPE) {
                w->should_close = true;
                return 0;
            }

            char mapped = 0;
            switch (wParam) {
            case VK_SPACE:   mapped = KEY_SPACE; break;
            case VK_RETURN:  mapped = KEY_ENTER; break;
            case VK_UP:      mapped = KEY_UP; break;
            case VK_DOWN:    mapped = KEY_DOWN; break;
            case VK_LEFT:    mapped = KEY_LEFT; break;
            case VK_RIGHT:   mapped = KEY_RIGHT; break;
            case 'C':        mapped = 'c'; break;
            case 'P':        mapped = 'p'; break;
            case 'F':        mapped = 'f'; break;
            case 'T':        mapped = 't'; break;
            case 'R':        mapped = 'r'; break;
            default: break;
            }

            if (mapped) {
                w->key_buf[w->key_head % KEY_BUF] = mapped;
                w->key_head++;
            }
        }
        return 0;

    case WM_CHAR:
        if (w && wParam >= 32 && wParam <= 126) {
            char c = (char)wParam;
            if (c != ' ' && c != 'c' && c != 'C' && c != 'p' && c != 'P' &&
                c != 'f' && c != 'F' && c != 't' && c != 'T' && c != 'r' && c != 'R') {
                w->key_buf[w->key_head % KEY_BUF] = c;
                w->key_head++;
            }
        }
        return 0;

    case WM_CLOSE:
    case WM_DESTROY:
        if (w) w->should_close = true;
        return 0;

    case WM_MOUSEWHEEL: {
        if (w) {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            w->wheel_delta += (delta > 0) ? 1 : -1;
        }
        return 0;
    }

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

PetWindow *window_create(const char *title, int w, int h) {
    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASSEX wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = pet_wnd_proc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "CortexPetWindow";
    RegisterClassEx(&wc);

    /* Posição inicial no canto inferior direito da área útil */
    RECT rcWork;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);
    int init_x = rcWork.right - w - 20;
    int init_y = rcWork.bottom - h - 40;

    /* Janela flutuante, sem bordas, com transparência por pixel */
    DWORD exStyle = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
    DWORD style   = WS_POPUP;

    HWND hwnd = CreateWindowEx(
        exStyle,
        "CortexPetWindow",
        title ? title : "Cortex Pet",
        style,
        init_x, init_y, w, h,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        fprintf(stderr, "[Win32] Falha ao criar a janela com CreateWindowEx.\n");
        return NULL;
    }

    PetWindow *win = (PetWindow *)calloc(1, sizeof(PetWindow));
    if (!win) {
        DestroyWindow(hwnd);
        return NULL;
    }

    win->hwnd   = hwnd;
    win->width  = w;
    win->height = h;
    win->pos_x  = (double)init_x;
    win->pos_y  = (double)init_y;

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)win);

    /* Criação do DIB Section de 32 bits (ARGB) com altura negativa para top-down */
    HDC hdcScreen = GetDC(NULL);
    win->hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = w;
    bmi.bmiHeader.biHeight      = -h; /* Top-down: linha 0 é o topo */
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    win->hbmp = CreateDIBSection(win->hdcMem, &bmi, DIB_RGB_COLORS, (void **)&win->pixels, NULL, 0);
    ReleaseDC(NULL, hdcScreen);

    if (!win->hbmp || !win->pixels) {
        fprintf(stderr, "[Win32] Falha ao criar CreateDIBSection de 32-bit.\n");
        free(win);
        DestroyWindow(hwnd);
        return NULL;
    }

    win->hbmpOld = (HBITMAP)SelectObject(win->hdcMem, win->hbmp);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    g_active_win = win;
    return win;
}

void window_destroy(PetWindow *w) {
    if (!w) return;
    if (g_active_win == w) g_active_win = NULL;

    if (w->hdcMem) {
        SelectObject(w->hdcMem, w->hbmpOld);
        DeleteDC(w->hdcMem);
    }
    if (w->hbmp) {
        DeleteObject(w->hbmp);
    }
    if (w->hwnd) {
        DestroyWindow(w->hwnd);
    }
    free(w);
}

bool window_should_close(const PetWindow *w) {
    return w ? w->should_close : true;
}

void window_poll_events(PetWindow *w) {
    (void)w;
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void window_present(PetWindow *w, const Canvas canvas) {
    if (!w || !w->pixels || !w->hdcMem) return;

    uint32_t *dst = w->pixels;
    int dst_w = w->width;
    int dst_h = w->height;

    /* Escala bilinear suave e precisa de 128x96 para 192x144 com pré-multiplicação de Alpha */
    for (int y = 0; y < dst_h; ++y) {
        float src_yf = ((float)y + 0.5f) * ((float)CANVAS_H / (float)dst_h) - 0.5f;
        int y0 = (int)floorf(src_yf);
        int y1 = y0 + 1;
        float fy = src_yf - (float)y0;
        if (y0 < 0) y0 = 0;
        if (y1 >= CANVAS_H) y1 = CANVAS_H - 1;

        int row_offset = y * dst_w;

        for (int x = 0; x < dst_w; ++x) {
            float src_xf = ((float)x + 0.5f) * ((float)CANVAS_W / (float)dst_w) - 0.5f;
            int x0 = (int)floorf(src_xf);
            int x1 = x0 + 1;
            float fx = src_xf - (float)x0;
            if (x0 < 0) x0 = 0;
            if (x1 >= CANVAS_W) x1 = CANVAS_W - 1;

            Pixel p00 = canvas[y0 * CANVAS_W + x0];
            Pixel p10 = canvas[y0 * CANVAS_W + x1];
            Pixel p01 = canvas[y1 * CANVAS_W + x0];
            Pixel p11 = canvas[y1 * CANVAS_W + x1];

            float w00 = (1.0f - fx) * (1.0f - fy);
            float w10 = fx * (1.0f - fy);
            float w01 = (1.0f - fx) * fy;
            float w11 = fx * fy;

            float r = (float)(p00 & 0xff)*w00 + (float)(p10 & 0xff)*w10 +
                      (float)(p01 & 0xff)*w01 + (float)(p11 & 0xff)*w11;
            float g = (float)((p00>>8) & 0xff)*w00 + (float)((p10>>8) & 0xff)*w10 +
                      (float)((p01>>8) & 0xff)*w01 + (float)((p11>>8) & 0xff)*w11;
            float b = (float)((p00>>16) & 0xff)*w00 + (float)((p10>>16) & 0xff)*w10 +
                      (float)((p01>>16) & 0xff)*w01 + (float)((p11>>16) & 0xff)*w11;
            float a = (float)((p00>>24) & 0xff)*w00 + (float)((p10>>24) & 0xff)*w10 +
                      (float)((p01>>24) & 0xff)*w01 + (float)((p11>>24) & 0xff)*w11;

            float fa = a * (1.0f / 255.0f);
            uint32_t ir = (uint32_t)(r * fa);
            uint32_t ig = (uint32_t)(g * fa);
            uint32_t ib = (uint32_t)(b * fa);
            uint32_t ia = (uint32_t)a;

            /* Formato DIB 32-bit: 0xAARRGGBB */
            dst[row_offset + x] = (ia << 24) | (ir << 16) | (ig << 8) | ib;
        }
    }

    /* Atualização atômica da janela transparente com aceleração DWM via UpdateLayeredWindow */
    HDC hdcScreen = GetDC(NULL);
    POINT ptSrc   = {0, 0};
    SIZE  sizeWnd = {dst_w, dst_h};
    POINT ptDst   = {(LONG)round(w->pos_x), (LONG)round(w->pos_y)};

    BLENDFUNCTION blend = {0};
    blend.BlendOp             = AC_SRC_OVER;
    blend.BlendFlags          = 0;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat         = AC_SRC_ALPHA; /* Transparência por pixel real */

    UpdateLayeredWindow(
        w->hwnd,
        hdcScreen,
        &ptDst,
        &sizeWnd,
        w->hdcMem,
        &ptSrc,
        0,
        &blend,
        ULW_ALPHA
    );

    ReleaseDC(NULL, hdcScreen);
}

void window_swap_buffers(PetWindow *w) {
    (void)w;
}

char window_read_char(PetWindow *w) {
    if (!w || w->key_head == w->key_tail) return 0;
    char c = w->key_buf[w->key_tail % KEY_BUF];
    w->key_tail++;
    return c;
}

void window_update_physics(PetWindow *w, double dt) {
    if (!w || dt <= 0.0001) return;
    if (dt > 0.1) dt = 0.1;

    int min_x = 0, min_y = 0, scr_w = 1920, scr_h = 1080;
    window_get_workarea(&min_x, &min_y, &scr_w, &scr_h);
    int max_x = min_x + scr_w - w->width;
    int max_y = min_y + scr_h - w->height;

    if (w->mouse_down) {
        POINT pt;
        if (GetCursorPos(&pt)) {
            int target_wx = pt.x - w->grab_ox;
            int target_wy = pt.y - w->grab_oy;

            double moved = hypot((double)(pt.x - w->press_x), (double)(pt.y - w->press_y));
            if (moved > 5.0) {
                w->is_click = false;
                w->dragging = true;
            }

            if (w->dragging) {
                double inst_vx = (target_wx - w->pos_x) / dt;
                double inst_vy = (target_wy - w->pos_y) / dt;
                w->vx = w->vx * 0.35 + inst_vx * 0.65;
                w->vy = w->vy * 0.35 + inst_vy * 0.65;
                w->pos_x = (double)target_wx;
                w->pos_y = (double)target_wy;

                SetWindowPos(w->hwnd, HWND_TOPMOST, target_wx, target_wy, 0, 0,
                             SWP_NOSIZE | SWP_NOACTIVATE);
            }
        }
        return;
    }

    if (w->in_physics) {
        w->vy += 920.0 * dt; /* Gravidade */

        double drag = pow(0.978, dt * 60.0); /* Resistência do ar */
        w->vx *= drag;

        w->pos_x += w->vx * dt;
        w->pos_y += w->vy * dt;

        if (w->pos_x < min_x) {
            w->pos_x = min_x;
            if (fabs(w->vx) > 70.0) w->bumped = true;
            w->vx = -w->vx * 0.78;
        } else if (w->pos_x > max_x) {
            w->pos_x = max_x;
            if (fabs(w->vx) > 70.0) w->bumped = true;
            w->vx = -w->vx * 0.78;
        }

        if (w->pos_y < min_y) {
            w->pos_y = min_y;
            if (fabs(w->vy) > 70.0) w->bumped = true;
            w->vy = -w->vy * 0.65;
        } else if (w->pos_y >= max_y) {
            w->pos_y = max_y;
            if (fabs(w->vy) > 70.0) w->bumped = true;
            w->vy = -w->vy * 0.74;
            w->vx *= 0.92;

            if (fabs(w->vy) < 14.0 && fabs(w->vx) < 12.0) {
                w->vx = 0.0;
                w->vy = 0.0;
                w->pos_y = max_y;
                w->in_physics = false;
            }
        }

        SetWindowPos(w->hwnd, HWND_TOPMOST, (int)round(w->pos_x), (int)round(w->pos_y), 0, 0,
                     SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

void window_get_pos(const PetWindow *w, int *out_x, int *out_y) {
    if (!w) return;
    if (out_x) *out_x = (int)round(w->pos_x);
    if (out_y) *out_y = (int)round(w->pos_y);
}

void window_get_workarea(int *out_x, int *out_y, int *out_w, int *out_h) {
    RECT rc;
    if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0)) {
        if (out_x) *out_x = rc.left;
        if (out_y) *out_y = rc.top;
        if (out_w) *out_w = rc.right - rc.left;
        if (out_h) *out_h = rc.bottom - rc.top;
    } else {
        if (out_x) *out_x = 0;
        if (out_y) *out_y = 0;
        if (out_w) *out_w = GetSystemMetrics(SM_CXSCREEN);
        if (out_h) *out_h = GetSystemMetrics(SM_CYSCREEN);
    }
}

bool window_was_bumped(PetWindow *w) {
    if (!w) return false;
    bool b = w->bumped;
    w->bumped = false;
    return b;
}

bool window_was_clicked(PetWindow *w) {
    if (!w) return false;
    bool b = w->was_clicked;
    w->was_clicked = false;
    return b;
}

bool window_is_dragging(const PetWindow *w) {
    return w && w->dragging;
}

double window_get_speed(const PetWindow *w) {
    if (!w) return 0.0;
    return hypot(w->vx, w->vy);
}

double window_get_vx(const PetWindow *w) {
    return w ? w->vx : 0.0;
}

double window_get_vy(const PetWindow *w) {
    return w ? w->vy : 0.0;
}

void window_apply_impulse(PetWindow *w, double fx, double fy) {
    if (!w) return;
    w->vx += fx;
    w->vy += fy;
    w->in_physics = true;
}

/* Scroll wheel — Win32 */
int window_get_wheel(PetWindow *w) {
    if (!w) return 0;
    int d = w->wheel_delta;
    w->wheel_delta = 0;
    return d;
}

/* Janela ativa — Win32 via GetForegroundWindow */
bool window_get_active_window_rect(PetWindow *w,
                                   int *out_x, int *out_y,
                                   int *out_ww, int *out_hh,
                                   uint64_t *out_id) {
    if (!w) return false;
    HWND fg = GetForegroundWindow();
    if (!fg || fg == w->hwnd) return false;
    RECT rc;
    if (!GetWindowRect(fg, &rc)) return false;
    if (out_x)  *out_x  = (int)rc.left;
    if (out_y)  *out_y  = (int)rc.top;
    if (out_ww) *out_ww = (int)(rc.right - rc.left);
    if (out_hh) *out_hh = (int)(rc.bottom - rc.top);
    if (out_id) *out_id = (uint64_t)(uintptr_t)fg;
    return true;
}