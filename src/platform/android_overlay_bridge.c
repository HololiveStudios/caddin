#include <jni.h>
#include <android/bitmap.h>
#include <android/log.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "renderer/pixel_draw.h"
#include "neuro.h"
#include "core/ontogeny.h"

#define LOG_TAG "CaddinOverlay"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

/* Referências globais */
static Canvas g_canvas;
static Brain *g_brain = NULL;
static Ontogeny g_ontogeny;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    (void)vm; (void)reserved;
    LOGI("JNI_OnLoad: libcaddin.so carregada com sucesso!");
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL
Java_org_hololive_caddin_CaddinOverlayService_nativeInit(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz;
    LOGI("nativeInit: Inicializando ecossistema Caddin");
    memset(g_canvas, 0, sizeof(g_canvas));
    if (!g_brain) {
        g_brain = brain_create(42, "android");
    }
    onto_init(&g_ontogeny, NULL);
}

JNIEXPORT void JNICALL
Java_org_hololive_caddin_CaddinOverlayService_nativeStep(JNIEnv *env, jclass clazz, jfloat dt) {
    (void)env; (void)clazz;
    (void)dt;
}

JNIEXPORT void JNICALL
Java_org_hololive_caddin_CaddinOverlayService_nativeRender(JNIEnv *env, jclass clazz, jobject bitmap) {
    (void)clazz;
    AndroidBitmapInfo info;
    void *pixels;

    if (AndroidBitmap_getInfo(env, bitmap, &info) < 0) return;
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) return;

    uint32_t *dst = (uint32_t *)pixels;
    int bw = info.width;
    int bh = info.height;
    int stride = info.stride / 4;

    for (int y = 0; y < bh; y++) {
        uint32_t *line = dst + y * stride;
        for (int x = 0; x < bw; x++) {
            line[x] = 0; /* Transparente */
        }
    }

    int cx = bw / 2;
    int cy = bh / 2;
    int r = (bw < bh ? bw : bh) / 2 - 8;
    int r2 = r * r;

    for (int y = 0; y < bh; y++) {
        uint32_t *line = dst + y * stride;
        int dy = y - cy;
        for (int x = 0; x < bw; x++) {
            int dx = x - cx;
            int d2 = dx * dx + dy * dy;
            if (d2 <= r2) {
                if (d2 >= (r - 4) * (r - 4)) {
                    line[x] = 0xFF2E3440;
                } else {
                    line[x] = 0xFFECEFF4;
                }
            }
        }
    }

    int eye_r = 6;
    int eye_y = cy - 4;
    int eye_x1 = cx - 14;
    int eye_x2 = cx + 14;

    for (int y = 0; y < bh; y++) {
        uint32_t *line = dst + y * stride;
        int dy = y - eye_y;
        for (int x = 0; x < bw; x++) {
            int dx1 = x - eye_x1;
            int dx2 = x - eye_x2;
            if ((dx1 * dx1 + dy * dy) <= eye_r * eye_r || (dx2 * dx2 + dy * dy) <= eye_r * eye_r) {
                line[x] = 0xFF2E3440;
            }
        }
    }

    AndroidBitmap_unlockPixels(env, bitmap);
}

JNIEXPORT void JNICALL
Java_org_hololive_caddin_CaddinOverlayService_nativeTouch(JNIEnv *env, jclass clazz, jfloat x, jfloat y, jint action) {
    (void)env; (void)clazz;
    LOGI("nativeTouch: x=%.1f, y=%.1f, action=%d", x, y, action);
}
