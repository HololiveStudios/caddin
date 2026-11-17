#ifndef CORTEX_X11_CURSOR_H
#define CORTEX_X11_CURSOR_H

#include <stdbool.h>

/* Consulta coordenadas absolutas globais do mouse na tela via X11 */
bool x11_get_screen_cursor(int *out_x, int *out_y);

#endif /* CORTEX_X11_CURSOR_H */
