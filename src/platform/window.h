#ifndef CORTEX_WINDOW_H
#define CORTEX_WINDOW_H

#include <stdbool.h>
#include <stdint.h>
#include "pixel_draw.h"

typedef struct PetWindow PetWindow;

/* Teclas especiais mapeadas em window_read_char */
#define KEY_SPACE   ' '
#define KEY_ENTER   '\n'
#define KEY_UP      (char)0x80
#define KEY_DOWN    (char)0x81
#define KEY_LEFT    (char)0x82
#define KEY_RIGHT   (char)0x83

PetWindow *window_create(const char *title, int w, int h);
void       window_destroy(PetWindow *w);
bool       window_should_close(const PetWindow *w);
void       window_poll_events(PetWindow *w);
void       window_swap_buffers(PetWindow *w);

/* Apresenta o canvas renderizado na janela via XShm / blit nativo com escala suave 1.5x */
void       window_present(PetWindow *w, const Canvas canvas);

/* Lê próximo char / evento de tecla digitado (0 = vazio) */
char       window_read_char(PetWindow *w);

/* Atualização de física de arrastar, soltar, arremesso e gravidade */
void       window_update_physics(PetWindow *w, double dt);
bool       window_was_bumped(PetWindow *w);
bool       window_was_clicked(PetWindow *w);
bool       window_is_dragging(const PetWindow *w);
double     window_get_speed(const PetWindow *w);
double     window_get_vx(const PetWindow *w);
double     window_get_vy(const PetWindow *w);
void       window_get_pos(const PetWindow *w, int *out_x, int *out_y);
void       window_get_workarea(int *out_x, int *out_y, int *out_w, int *out_h);
void       window_apply_impulse(PetWindow *w, double fx, double fy);

#endif /* CORTEX_WINDOW_H */
