#ifndef CORTEX_X11_CURSOR_H
#define CORTEX_X11_CURSOR_H

#include <stdbool.h>

/* Consulta coordenadas absolutas globais do mouse na tela via X11 */
bool x11_get_screen_cursor(int *out_x, int *out_y);

/* Retorna o Display* singleton (como void*) e o root Window id — Linux apenas.
 * Declarados em x11_cursor.c como extern "Display*" mas expostos como void*
 * para não exigir inclusão de Xlib nos clientes. */
#if defined(__linux__) || defined(__unix__)
void         *x11_get_display(void);
unsigned long x11_get_root(void);
#endif

#endif /* CORTEX_X11_CURSOR_H */
