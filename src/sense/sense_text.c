#include "sense_text.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#if defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

void sense_text_init(SenseText *st) {
    if (!st) return;
    memset(st, 0, sizeof(*st));
}

static void extract_first_meaningful_word(const char *src, char *out, size_t max_len) {
    if (!src || !out || max_len == 0) return;
    out[0] = '\0';

    const char *p = src;
    /* Pula pontuação e espaços iniciais */
    while (*p && !isalnum((unsigned char)*p)) p++;

    size_t count = 0;
    while (*p && isalnum((unsigned char)*p) && count + 1 < max_len) {
        out[count++] = (char)tolower((unsigned char)*p);
        p++;
    }
    out[count] = '\0';
}

void sense_text_update(SenseText *st, float dt, void *native_display, unsigned long active_window) {
    if (!st) return;
    st->poll_timer += dt;
    if (st->poll_timer < 0.5f) return; /* Atualiza no máximo 2x por segundo */
    st->poll_timer = 0.0f;

    char new_title[SENSE_TEXT_MAX_LEN] = {0};

#if defined(__linux__)
    Display *dpy = (Display *)native_display;
    if (dpy && active_window != 0) {
        Atom prop = XInternAtom(dpy, "_NET_WM_NAME", True);
        Atom type = None;
        int format = 0;
        unsigned long nitems = 0, bytes_after = 0;
        unsigned char *prop_val = NULL;

        if (prop != None && XGetWindowProperty(dpy, (Window)active_window, prop, 0, 64,
                                               False, AnyPropertyType, &type, &format,
                                               &nitems, &bytes_after, &prop_val) == Success && prop_val) {
            strncpy(new_title, (char *)prop_val, sizeof(new_title) - 1);
            XFree(prop_val);
        } else {
            /* Fallback: WM_NAME padrão */
            char *name = NULL;
            if (XFetchName(dpy, (Window)active_window, &name) > 0 && name) {
                strncpy(new_title, name, sizeof(new_title) - 1);
                XFree(name);
            }
        }
    }
#elif defined(_WIN32)
    HWND hwnd = GetForegroundWindow();
    if (hwnd) {
        GetWindowTextA(hwnd, new_title, sizeof(new_title) - 1);
    }
#endif

    if (new_title[0] != '\0' && strcmp(new_title, st->current_title) != 0) {
        strncpy(st->current_title, new_title, sizeof(st->current_title) - 1);
        char candidate[64] = {0};
        extract_first_meaningful_word(new_title, candidate, sizeof(candidate));
        if (candidate[0] != '\0' && strcmp(candidate, st->last_word) != 0) {
            strncpy(st->last_word, candidate, sizeof(st->last_word) - 1);
            st->has_new_word = true;
        }
    }
}

const char *sense_text_get_last_word(const SenseText *st) {
    return st ? st->last_word : "";
}

bool sense_text_has_new_word(SenseText *st) {
    if (!st || !st->has_new_word) return false;
    st->has_new_word = false;
    return true;
}
