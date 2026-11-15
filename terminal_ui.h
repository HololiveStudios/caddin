#ifndef PESTINHA_TERMINAL_UI_H
#define PESTINHA_TERMINAL_UI_H

#include "runtime_types.h"
#include "neuro.h"

void terminal_ui_init(TerminalState *term);
void terminal_ui_cleanup(TerminalState *term);
void terminal_ui_render(const Brain *brain, const Drives *drives, const Emotion *emotion,
                        const Behavior *behavior, const FaceState *face,
                        const ConditioningState *cond, double fps, double now_t);
char terminal_ui_read_key(void);

#endif /* PESTINHA_TERMINAL_UI_H */
