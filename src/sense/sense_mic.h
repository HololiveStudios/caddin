#ifndef PET_SENSE_MIC_H
#define PET_SENSE_MIC_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
    bool     available;
    bool     enabled;
    uint32_t sample_rate;
    float    current_rms;      /* Volume atual normalizado [0.0 .. 1.0] */
    float    peak_rms;         /* Pico recente */
    bool     clapped;          /* Detectou palma/estalo brusco */
    void    *backend_handle;   /* snd_pcm_t* (Linux ALSA) ou HWAVEIN (Windows) */
} SenseMic;

/* Inicializa o subsistema de captura de áudio ambiente */
bool  sense_mic_init(SenseMic *mic, uint32_t sample_rate);

/* Captura amostras pendentes em modo não-bloqueante e calcula RMS */
void  sense_mic_update(SenseMic *mic, float dt);

/* Retorna energia sonora atual [0.0 .. 1.0] */
float sense_mic_get_rms(const SenseMic *mic);

/* Retorna true se houver um som alto repentino (ex: palma ou grito) */
bool  sense_mic_is_loud(const SenseMic *mic);

/* Libera recursos e fecha streams */
void  sense_mic_destroy(SenseMic *mic);

#endif /* PET_SENSE_MIC_H */
