#include "window.h"
#include "config.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#define KEY_BUF 64

struct PetWindow {
    Display *dpy;
    int screen;
    Window xwin;
    GC gc;
    Visual *visual;
    int depth;

    int width, height;

    /* XShm */
    bool has_shm;
    XShmSegmentInfo shminfo;
    XImage *ximage;

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
    Atom wm_delete_window;

    /* fila circular de teclas */
    char  key_buf[KEY_BUF];
    int   key_head, key_tail;

    /* scroll wheel — acumulado entre frames */
    int   wheel_delta;
};

static struct PetWindow *g_active_win = NULL;

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

PetWindow *window_create(const char *title, int w, int h) {
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "[X11] Não foi possível abrir o Display padrão.\n");
        return NULL;
    }

    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);

    /* 1. Busca Visual TrueColor de 32-bit para transparência real ARGB */
    XVisualInfo vinfo;
    bool found_32 = XMatchVisualInfo(dpy, screen, 32, TrueColor, &vinfo);
    if (!found_32) {
        fprintf(stderr, "[X11] Visual 32-bit não encontrado, usando visual padrão.\n");
        XMatchVisualInfo(dpy, screen, DefaultDepth(dpy, screen), TrueColor, &vinfo);
    }

    Colormap cmap = XCreateColormap(dpy, root, vinfo.visual, AllocNone);
    XSetWindowAttributes attrs;
    memset(&attrs, 0, sizeof(attrs));
    attrs.colormap = cmap;
    attrs.border_pixel = 0;
    attrs.background_pixel = 0;
    attrs.override_redirect = False;
    attrs.event_mask = StructureNotifyMask | ExposureMask |
                       KeyPressMask | KeyReleaseMask |
                       ButtonPressMask | ButtonReleaseMask |
                       PointerMotionMask;

    unsigned long mask = CWColormap | CWBorderPixel | CWBackPixel | CWEventMask;

    /* Posição inicial no canto inferior direito */
    int init_x = DisplayWidth(dpy, screen) - w - 20;
    int init_y = DisplayHeight(dpy, screen) - h - 40;

    /* Tenta consultar área útil (_NET_WORKAREA) */
    Atom net_workarea = XInternAtom(dpy, "_NET_WORKAREA", True);
    if (net_workarea != None) {
        Atom actual_type;
        int actual_format;
        unsigned long nitems, bytes_after;
        long *workarea = NULL;
        if (XGetWindowProperty(dpy, root, net_workarea, 0, 4, False,
                               XA_CARDINAL, &actual_type, &actual_format, &nitems,
                               &bytes_after, (unsigned char **)&workarea) == Success) {
            if (workarea && nitems >= 4) {
                init_x = (int)workarea[0] + (int)workarea[2] - w - 20;
                init_y = (int)workarea[1] + (int)workarea[3] - h - 40;
                XFree(workarea);
            }
        }
    }

    Window xwin = XCreateWindow(dpy, root, init_x, init_y, w, h, 0,
                                vinfo.depth, InputOutput, vinfo.visual,
                                mask, &attrs);
    if (!xwin) {
        XCloseDisplay(dpy);
        return NULL;
    }

    /* 2. Define título */
    XStoreName(dpy, xwin, title ? title : "Cortex Pet");

    /* 3. Janela sem bordas (Motif WM hints: decorations = 0) */
    struct {
        unsigned long flags;
        unsigned long functions;
        unsigned long decorations;
        long inputMode;
        unsigned long status;
    } mwm = { 2, 0, 0, 0, 0 }; /* flags = MWM_HINTS_DECORATIONS (2), decorations = 0 */
    Atom mwm_atom = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
    XChangeProperty(dpy, xwin, mwm_atom, mwm_atom, 32, PropModeReplace, (unsigned char *)&mwm, 5);

    /* 4. Janela flutuante no topo e fora da barra de tarefas */
    Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
    Atom state_above = XInternAtom(dpy, "_NET_WM_STATE_ABOVE", False);
    Atom state_skip_taskbar = XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", False);
    Atom state_skip_pager = XInternAtom(dpy, "_NET_WM_STATE_SKIP_PAGER", False);
    Atom states[3] = { state_above, state_skip_taskbar, state_skip_pager };
    XChangeProperty(dpy, xwin, wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char *)states, 3);

    Atom wm_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    Atom type_utility = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_UTILITY", False);
    XChangeProperty(dpy, xwin, wm_type, XA_ATOM, 32, PropModeReplace, (unsigned char *)&type_utility, 1);

    /* 5. Protocolo de fechamento limpo */
    Atom wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, xwin, &wm_delete, 1);

    /* 6. Cria GC */
    GC gc = XCreateGC(dpy, xwin, 0, NULL);

    PetWindow *win = calloc(1, sizeof(PetWindow));
    if (!win) {
        XDestroyWindow(dpy, xwin);
        XCloseDisplay(dpy);
        return NULL;
    }

    win->dpy              = dpy;
    win->screen           = screen;
    win->xwin             = xwin;
    win->gc               = gc;
    win->visual           = vinfo.visual;
    win->depth            = vinfo.depth;
    win->width            = w;
    win->height           = h;
    win->pos_x            = (double)init_x;
    win->pos_y            = (double)init_y;
    win->wm_delete_window = wm_delete;

    /* 7. Inicializa XShm para transferência ultra-rápida de memória */
    win->has_shm = false;
    if (XShmQueryExtension(dpy)) {
        win->ximage = XShmCreateImage(dpy, vinfo.visual, vinfo.depth, ZPixmap, NULL, &win->shminfo, w, h);
        if (win->ximage) {
            win->shminfo.shmid = shmget(IPC_PRIVATE, win->ximage->bytes_per_line * win->ximage->height, IPC_CREAT | 0777);
            if (win->shminfo.shmid >= 0) {
                win->shminfo.shmaddr = win->ximage->data = (char *)shmat(win->shminfo.shmid, 0, 0);
                if (win->shminfo.shmaddr != (char *)-1) {
                    win->shminfo.readOnly = False;
                    if (XShmAttach(dpy, &win->shminfo)) {
                        shmctl(win->shminfo.shmid, IPC_RMID, 0);
                        win->has_shm = true;
                    } else {
                        shmdt(win->shminfo.shmaddr);
                        shmctl(win->shminfo.shmid, IPC_RMID, 0);
                        XDestroyImage(win->ximage);
                        win->ximage = NULL;
                    }
                } else {
                    shmctl(win->shminfo.shmid, IPC_RMID, 0);
                    XDestroyImage(win->ximage);
                    win->ximage = NULL;
                }
            } else {
                XDestroyImage(win->ximage);
                win->ximage = NULL;
            }
        }
    }

    /* Fallback padrão se XShm não estiver disponível */
    if (!win->has_shm) {
        char *img_data = (char *)malloc(w * h * 4);
        win->ximage = XCreateImage(dpy, vinfo.visual, vinfo.depth, ZPixmap, 0, img_data, w, h, 32, 0);
    }

    /* 8. Mapeia a janela na tela */
    XMapWindow(dpy, xwin);
    XFlush(dpy);

    g_active_win = win;
    return win;
}

void window_destroy(PetWindow *w) {
    if (!w) return;
    if (g_active_win == w) g_active_win = NULL;

    if (w->ximage) {
        if (w->has_shm) {
            XShmDetach(w->dpy, &w->shminfo);
            XDestroyImage(w->ximage);
            shmdt(w->shminfo.shmaddr);
        } else {
            XDestroyImage(w->ximage);
        }
        w->ximage = NULL;
    }

    if (w->gc) XFreeGC(w->dpy, w->gc);
    if (w->xwin) XDestroyWindow(w->dpy, w->xwin);
    if (w->dpy) XCloseDisplay(w->dpy);
    free(w);
}

bool window_should_close(const PetWindow *w) {
    return w ? w->should_close : true;
}

void window_poll_events(PetWindow *w) {
    if (!w || !w->dpy) return;

    while (XPending(w->dpy)) {
        XEvent event;
        XNextEvent(w->dpy, &event);

        switch (event.type) {
        case ClientMessage:
            if (event.xclient.data.l[0] == (long)w->wm_delete_window) {
                w->should_close = true;
            }
            break;

        case ButtonPress:
            /* Scroll wheel: Button4 = cima, Button5 = baixo */
            if (event.xbutton.button == Button4) {
                w->wheel_delta += 1;
                break;
            }
            if (event.xbutton.button == Button5) {
                w->wheel_delta -= 1;
                break;
            }
            if (event.xbutton.button == Button1) {
                w->mouse_down = true;
                w->dragging   = false;
                w->is_click   = true;
                w->press_time = now_sec();
                w->in_physics = false;
                w->vx = 0.0;
                w->vy = 0.0;

                /* Posição do clique relativo ao canto da própria janela */
                w->grab_ox = event.xbutton.x;
                w->grab_oy = event.xbutton.y;

                /* Posição absoluta do clique na tela */
                w->press_x = event.xbutton.x_root;
                w->press_y = event.xbutton.y_root;

                /* Consulta coordenadas reais da janela na tela (root) */
                int root_x = 0, root_y = 0;
                Window child_dummy;
                XTranslateCoordinates(w->dpy, w->xwin, RootWindow(w->dpy, w->screen),
                                      0, 0, &root_x, &root_y, &child_dummy);
                w->pos_x = (double)root_x;
                w->pos_y = (double)root_y;
            }
            break;

        case ButtonRelease:
            if (event.xbutton.button == Button1) {
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
            break;

        case KeyPress: {
            KeySym sym = XLookupKeysym(&event.xkey, 0);
            if (sym == XK_Escape) {
                w->should_close = true;
                break;
            }

            char mapped = 0;
            switch (sym) {
            case XK_space:    mapped = KEY_SPACE; break;
            case XK_Return:
            case XK_KP_Enter: mapped = KEY_ENTER; break;
            case XK_Up:       mapped = KEY_UP; break;
            case XK_Down:     mapped = KEY_DOWN; break;
            case XK_Left:     mapped = KEY_LEFT; break;
            case XK_Right:    mapped = KEY_RIGHT; break;
            case XK_c: case XK_C: mapped = 'c'; break;
            case XK_p: case XK_P: mapped = 'p'; break;
            case XK_f: case XK_F: mapped = 'f'; break;
            case XK_t: case XK_T: mapped = 't'; break;
            case XK_r: case XK_R: mapped = 'r'; break;
            default: {
                char buf[16];
                int len = XLookupString(&event.xkey, buf, sizeof(buf), NULL, NULL);
                if (len > 0 && buf[0] >= 32 && buf[0] <= 126) {
                    mapped = buf[0];
                }
                break;
            }
            }

            if (mapped) {
                w->key_buf[w->key_head % KEY_BUF] = mapped;
                w->key_head++;
            }
            break;
        }

        default:
            break;
        }
    }
}

void window_present(PetWindow *w, const Canvas canvas) {
    if (!w || !w->ximage || !w->ximage->data) return;

    uint32_t *dst = (uint32_t *)w->ximage->data;
    int dst_w = w->width;
    int dst_h = w->height;
    int bpl32 = w->ximage->bytes_per_line / 4;

    /* ── Escala bilinear suave e precisa de 128x96 para 192x144 ── */
    for (int y = 0; y < dst_h; ++y) {
        float src_yf = ((float)y + 0.5f) * ((float)CANVAS_H / (float)dst_h) - 0.5f;
        int y0 = (int)floorf(src_yf);
        int y1 = y0 + 1;
        float fy = src_yf - (float)y0;
        if (y0 < 0) y0 = 0;
        if (y1 >= CANVAS_H) y1 = CANVAS_H - 1;

        int row_offset = y * bpl32;

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

            /* Pesos bilineares */
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

            /* Pre-multiplica alpha para composição correta em compositores X11 */
            float fa = a * (1.0f / 255.0f);
            uint32_t ir = (uint32_t)(r * fa);
            uint32_t ig = (uint32_t)(g * fa);
            uint32_t ib = (uint32_t)(b * fa);
            uint32_t ia = (uint32_t)a;

            dst[row_offset + x] = (ia << 24) | (ir << 16) | (ig << 8) | ib;
        }
    }

    /* Blit direto via XShm (zero cópias de GPU) */
    if (w->has_shm) {
        XShmPutImage(w->dpy, w->xwin, w->gc, w->ximage, 0, 0, 0, 0, dst_w, dst_h, False);
    } else {
        XPutImage(w->dpy, w->xwin, w->gc, w->ximage, 0, 0, 0, 0, dst_w, dst_h);
    }
    XFlush(w->dpy);
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

    int min_x = 0, min_y = 0, max_x = 1920 - w->width, max_y = 1080 - w->height;
    int scr_w = 1920, scr_h = 1080;
    window_get_workarea(&min_x, &min_y, &scr_w, &scr_h);
    max_x = min_x + scr_w - w->width;
    max_y = min_y + scr_h - w->height;

    if (w->mouse_down) {
        Window root_ret, child_ret;
        int rx = 0, ry = 0, wx_pos = 0, wy_pos = 0;
        unsigned int mask = 0;
        if (XQueryPointer(w->dpy, RootWindow(w->dpy, w->screen),
                          &root_ret, &child_ret, &rx, &ry, &wx_pos, &wy_pos, &mask)) {
            int target_wx = rx - w->grab_ox;
            int target_wy = ry - w->grab_oy;

            double moved = hypot((double)(rx - w->press_x), (double)(ry - w->press_y));
            if (moved > 5.0) {
                w->is_click = false;
                w->dragging = true;
            }

            if (w->dragging) {
                XMoveWindow(w->dpy, w->xwin, target_wx, target_wy);
                XFlush(w->dpy);

                double inst_vx = (target_wx - w->pos_x) / dt;
                double inst_vy = (target_wy - w->pos_y) / dt;
                w->vx = w->vx * 0.35 + inst_vx * 0.65;
                w->vy = w->vy * 0.35 + inst_vy * 0.65;
                w->pos_x = (double)target_wx;
                w->pos_y = (double)target_wy;
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
            w->vx = -w->vx * 0.78; /* Quique elástico lateral */
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
            w->vy = -w->vy * 0.74; /* Quique elástico no chão */
            w->vx *= 0.92;

            if (fabs(w->vy) < 14.0 && fabs(w->vx) < 12.0) {
                w->vx = 0.0;
                w->vy = 0.0;
                w->pos_y = max_y;
                w->in_physics = false;
            }
        }

        XMoveWindow(w->dpy, w->xwin, (int)round(w->pos_x), (int)round(w->pos_y));
    }
}

void window_get_pos(const PetWindow *w, int *out_x, int *out_y) {
    if (!w) return;
    if (out_x) *out_x = (int)round(w->pos_x);
    if (out_y) *out_y = (int)round(w->pos_y);
}

void window_get_workarea(int *out_x, int *out_y, int *out_w, int *out_h) {
    if (out_x) *out_x = 0;
    if (out_y) *out_y = 0;
    if (out_w) *out_w = 1920;
    if (out_h) *out_h = 1080;

    Display *dpy = g_active_win ? g_active_win->dpy : XOpenDisplay(NULL);
    if (!dpy) return;
    int scr = DefaultScreen(dpy);
    Window root = RootWindow(dpy, scr);

    Atom net_workarea = XInternAtom(dpy, "_NET_WORKAREA", True);
    if (net_workarea != None) {
        Atom actual_type;
        int actual_format;
        unsigned long nitems, bytes_after;
        long *workarea = NULL;
        if (XGetWindowProperty(dpy, root, net_workarea, 0, 4, False,
                               XA_CARDINAL, &actual_type, &actual_format, &nitems,
                               &bytes_after, (unsigned char **)&workarea) == Success) {
            if (workarea && nitems >= 4) {
                if (out_x) *out_x = (int)workarea[0];
                if (out_y) *out_y = (int)workarea[1];
                if (out_w) *out_w = (int)workarea[2];
                if (out_h) *out_h = (int)workarea[3];
                XFree(workarea);
                if (!g_active_win) XCloseDisplay(dpy);
                return;
            }
        }
    }

    if (out_w) *out_w = DisplayWidth(dpy, scr);
    if (out_h) *out_h = DisplayHeight(dpy, scr);
    if (!g_active_win) XCloseDisplay(dpy);
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

/* ── Fix 3: leitura acumulada do scroll wheel ── */
int window_get_wheel(PetWindow *w) {
    if (!w) return 0;
    int d = w->wheel_delta;
    w->wheel_delta = 0;
    return d;
}

/* ── Fix 4: retorna geometria da janela ativa no X11 via _NET_ACTIVE_WINDOW ── */
bool window_get_active_window_rect(PetWindow *w,
                                   int *out_x, int *out_y,
                                   int *out_ww, int *out_hh,
                                   uint64_t *out_id) {
    if (!w) return false;
    Display *dpy = w->dpy;
    int screen   = w->screen;
    Window root  = RootWindow(dpy, screen);

    Atom net_aw = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", True);
    if (net_aw == None) return false;

    Atom actual_type;
    int  actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop_data = NULL;

    int ret = XGetWindowProperty(dpy, root, net_aw, 0, 1, False, XA_WINDOW,
                                 &actual_type, &actual_format,
                                 &nitems, &bytes_after, &prop_data);
    if (ret != Success || !prop_data || nitems < 1) {
        if (prop_data) XFree(prop_data);
        return false;
    }

    Window active = *(Window *)prop_data;
    XFree(prop_data);

    /* Ignora a própria janela do pet */
    if (active == w->xwin || active == None) return false;

    /* Obtém geometria */
    XWindowAttributes xwa;
    if (!XGetWindowAttributes(dpy, active, &xwa)) return false;

    /* Converte coordenadas para root */
    int rx = 0, ry = 0;
    Window child_dummy;
    XTranslateCoordinates(dpy, active, root, 0, 0, &rx, &ry, &child_dummy);

    if (out_x)  *out_x  = rx;
    if (out_y)  *out_y  = ry;
    if (out_ww) *out_ww = xwa.width;
    if (out_hh) *out_hh = xwa.height;
    if (out_id) *out_id = (uint64_t)active;

    return true;
}
