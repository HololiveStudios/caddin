#ifndef PET_SENSE_KEYSTROKE_BIOMETRICS_H
#define PET_SENSE_KEYSTROKE_BIOMETRICS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define KEYSTROKE_BINS 16
#define KEYSTROKE_MIN_INTERVAL_MS 20
#define KEYSTROKE_MAX_INTERVAL_MS 1000

typedef struct {
    double   last_key_time_s;
    uint32_t current_histogram[KEYSTROKE_BINS];
    uint32_t current_samples;

    /* Perfil biométrico imprinting do dono */
    uint32_t owner_histogram[KEYSTROKE_BINS];
    uint32_t owner_samples;
    bool     owner_learned;

    float    last_similarity;
    uint32_t total_strokes;
} SenseKeystrokeBiometrics;

void sense_keystroke_init(SenseKeystrokeBiometrics *kb);

/* Registra um toque de tecla no tempo now_s */
void sense_keystroke_press(SenseKeystrokeBiometrics *kb, char key, double now_s);

/* Calcula similaridade de cosseno (0.0f a 1.0f) com o perfil do dono */
float sense_keystroke_get_owner_similarity(const SenseKeystrokeBiometrics *kb);

/* Consolidação do imprinting (grava o histograma atual como dono se tiver amostras suficientes) */
bool sense_keystroke_consolidate_imprint(SenseKeystrokeBiometrics *kb);

/* Serialização / desserialização compacta (sem malloc) */
size_t sense_keystroke_serialize(const SenseKeystrokeBiometrics *kb, uint8_t *buf, size_t max_size);
bool   sense_keystroke_deserialize(SenseKeystrokeBiometrics *kb, const uint8_t *buf, size_t size);

#endif /* PET_SENSE_KEYSTROKE_BIOMETRICS_H */
