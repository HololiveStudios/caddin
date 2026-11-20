#include "sense_keystroke_biometrics.h"
#include <string.h>
#include <math.h>

void sense_keystroke_init(SenseKeystrokeBiometrics *kb) {
    if (!kb) return;
    memset(kb, 0, sizeof(*kb));
    kb->last_key_time_s = -1.0;
    kb->last_similarity = 1.0f;
}

static int interval_to_bin(double interval_ms) {
    if (interval_ms < (double)KEYSTROKE_MIN_INTERVAL_MS) return 0;
    if (interval_ms >= (double)KEYSTROKE_MAX_INTERVAL_MS) return KEYSTROKE_BINS - 1;

    double range = (double)(KEYSTROKE_MAX_INTERVAL_MS - KEYSTROKE_MIN_INTERVAL_MS);
    double step = range / (double)KEYSTROKE_BINS;
    int bin = (int)((interval_ms - KEYSTROKE_MIN_INTERVAL_MS) / step);
    if (bin < 0) bin = 0;
    if (bin >= KEYSTROKE_BINS) bin = KEYSTROKE_BINS - 1;
    return bin;
}

void sense_keystroke_press(SenseKeystrokeBiometrics *kb, char key, double now_s) {
    if (!kb) return;
    (void)key;

    kb->total_strokes++;
    if (kb->last_key_time_s > 0.0) {
        double delta_ms = (now_s - kb->last_key_time_s) * 1000.0;
        /* Ignora pausas longas (> 2.0s) pois não representam a cadência de digitação contínua */
        if (delta_ms >= (double)KEYSTROKE_MIN_INTERVAL_MS && delta_ms <= 2000.0) {
            int bin = interval_to_bin(delta_ms);
            kb->current_histogram[bin]++;
            kb->current_samples++;

            /* Atualiza similaridade a cada ~16 amostras se já aprendeu */
            if (kb->owner_learned && (kb->current_samples % 16 == 0)) {
                kb->last_similarity = sense_keystroke_get_owner_similarity(kb);
            }
        }
    }
    kb->last_key_time_s = now_s;
}

float sense_keystroke_get_owner_similarity(const SenseKeystrokeBiometrics *kb) {
    if (!kb || !kb->owner_learned || kb->current_samples < 32 || kb->owner_samples < 32) {
        return 1.0f; /* Neutro / tolerante se ainda não tiver amostras suficientes */
    }

    double dot = 0.0;
    double norm_a = 0.0;
    double norm_b = 0.0;

    for (int i = 0; i < KEYSTROKE_BINS; ++i) {
        double a = (double)kb->current_histogram[i] / (double)kb->current_samples;
        double b = (double)kb->owner_histogram[i] / (double)kb->owner_samples;
        dot += a * b;
        norm_a += a * a;
        norm_b += b * b;
    }

    if (norm_a <= 1e-9 || norm_b <= 1e-9) return 1.0f;

    double sim = dot / (sqrt(norm_a) * sqrt(norm_b));
    if (sim < 0.0) sim = 0.0;
    if (sim > 1.0) sim = 1.0;
    return (float)sim;
}

bool sense_keystroke_consolidate_imprint(SenseKeystrokeBiometrics *kb) {
    if (!kb) return false;
    /* Exige pelo menos 100 amostras de digitação para consolidar */
    if (kb->current_samples < 100) return false;

    if (!kb->owner_learned) {
        /* Primeiro imprinting: grava diretamente */
        memcpy(kb->owner_histogram, kb->current_histogram, sizeof(kb->owner_histogram));
        kb->owner_samples = kb->current_samples;
        kb->owner_learned = true;
    } else {
        /* Atualização suave: EMA com o perfil consolidado */
        for (int i = 0; i < KEYSTROKE_BINS; ++i) {
            kb->owner_histogram[i] = (uint32_t)((kb->owner_histogram[i] * 3 + kb->current_histogram[i]) / 4);
        }
        kb->owner_samples = (kb->owner_samples * 3 + kb->current_samples) / 4;
    }

    /* Reseta o histograma de sessão para nova janela */
    memset(kb->current_histogram, 0, sizeof(kb->current_histogram));
    kb->current_samples = 0;
    kb->last_similarity = 1.0f;
    return true;
}

size_t sense_keystroke_serialize(const SenseKeystrokeBiometrics *kb, uint8_t *buf, size_t max_size) {
    if (!kb || !buf) return 0;
    size_t required = sizeof(uint32_t) * 2 + sizeof(kb->owner_histogram) + sizeof(uint32_t);
    if (max_size < required) return 0;

    size_t off = 0;
    uint32_t magic = 0x4B455953; /* 'KEYS' */
    memcpy(buf + off, &magic, sizeof(magic)); off += sizeof(magic);

    uint32_t learned = kb->owner_learned ? 1 : 0;
    memcpy(buf + off, &learned, sizeof(learned)); off += sizeof(learned);

    memcpy(buf + off, &kb->owner_samples, sizeof(kb->owner_samples)); off += sizeof(kb->owner_samples);
    memcpy(buf + off, kb->owner_histogram, sizeof(kb->owner_histogram)); off += sizeof(kb->owner_histogram);

    return off;
}

bool sense_keystroke_deserialize(SenseKeystrokeBiometrics *kb, const uint8_t *buf, size_t size) {
    if (!kb || !buf) return false;
    size_t required = sizeof(uint32_t) * 2 + sizeof(kb->owner_histogram) + sizeof(uint32_t);
    if (size < required) return false;

    size_t off = 0;
    uint32_t magic = 0;
    memcpy(&magic, buf + off, sizeof(magic)); off += sizeof(magic);
    if (magic != 0x4B455953) return false;

    uint32_t learned = 0;
    memcpy(&learned, buf + off, sizeof(learned)); off += sizeof(learned);
    kb->owner_learned = (learned != 0);

    memcpy(&kb->owner_samples, buf + off, sizeof(kb->owner_samples)); off += sizeof(kb->owner_samples);
    memcpy(kb->owner_histogram, buf + off, sizeof(kb->owner_histogram)); off += sizeof(kb->owner_histogram);

    return true;
}
