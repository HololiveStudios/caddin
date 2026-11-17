#include "x11_cursor.h"
#include <stdbool.h>

#if defined(_WIN32)
#include <windows.h>

bool x11_get_screen_cursor(int *out_x, int *out_y) {
    POINT pt;
    if (GetCursorPos(&pt)) {
        if (out_x) *out_x = (int)pt.x;
        if (out_y) *out_y = (int)pt.y;
        return true;
    }
    return false;
}

#elif defined(__linux__) || defined(__unix__)
#include <X11/Xlib.h>

static Display *s_cursor_dpy = NULL;

bool x11_get_screen_cursor(int *out_x, int *out_y) {
    if (!s_cursor_dpy) {
        s_cursor_dpy = XOpenDisplay(NULL);
        if (!s_cursor_dpy) return false;
    }

    Window root = DefaultRootWindow(s_cursor_dpy);
    Window root_ret, child_ret;
    int rx = 0, ry = 0, wx = 0, wy = 0;
    unsigned int mask = 0;

    if (XQueryPointer(s_cursor_dpy, root, &root_ret, &child_ret, &rx, &ry, &wx, &wy, &mask)) {
        if (out_x) *out_x = rx;
        if (out_y) *out_y = ry;
        return true;
    }
    return false;
}

#else

bool x11_get_screen_cursor(int *out_x, int *out_y) {
    (void)out_x; (void)out_y;
    return false;
}

#endif
