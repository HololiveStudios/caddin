#ifndef CORTEX_WINDOW_ANDROID_H
#define CORTEX_WINDOW_ANDROID_H

#include <android_native_app_glue.h>
#include <android/native_window.h>
#include "window.h"

void android_set_app_state(struct android_app *app);
struct android_app *android_get_app_state(void);

#endif
