#include <android_native_app_glue.h>
#include <android/log.h>
#include "window_android.h"

#define LOG_TAG "Caddin"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

/* Declaração da função principal do Caddin */
extern int caddin_app_main(int argc, char **argv);

static void handle_cmd(struct android_app *app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            LOGI("APP_CMD_INIT_WINDOW: Tela inicializada!");
            break;
        case APP_CMD_TERM_WINDOW:
            LOGI("APP_CMD_TERM_WINDOW");
            break;
    }
}

void android_main(struct android_app *state) {
    LOGI("android_main iniciado!");
    state->onAppCmd = handle_cmd;
    android_set_app_state(state);

    /* Aguarda a janela ser criada pelo Android antes de iniciar o loop */
    while (!state->window && !state->destroyRequested) {
        int events;
        struct android_poll_source *source;
        if (ALooper_pollOnce(-1, NULL, &events, (void **)&source) >= 0) {
            if (source != NULL) {
                source->process(state, source);
            }
        }
    }

    if (!state->destroyRequested) {
        char *argv[] = { "caddin", NULL };
        caddin_app_main(1, argv);
    }
}
