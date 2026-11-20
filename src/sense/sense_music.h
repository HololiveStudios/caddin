#ifndef PET_SENSE_MUSIC_H
#define PET_SENSE_MUSIC_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define MUSIC_HISTORY_LEN 32

typedef struct {
    bool     available;
    bool     is_music;          /* Se há música rítmica confirmada */
    float    music_confidence;  /* Nível de certeza [0.0 .. 1.0] */
    float    bpm;               /* Batimentos por minuto estimados (ex: 120.0f) */
    float    beat_interval;     /* Segundos por batida (ex: 0.5s para 120 BPM) */
    float    beat_timer;        /* Temporizador acumulado do compasso */
    float    beat_phase;        /* Fase do compasso [0.0 .. 1.0] */
    bool     is_on_beat;        /* True no instante exato do pico da batida */
    float    beat_intensity;    /* Força da batida atual [0.0 .. 1.0] */
    float    music_energy;      /* Volume / energia sonora [0.0 .. 1.0] */

    /* Rastreamento de batidas (DSP Onset Detection) */
    float    prev_energy;
    float    energy_threshold;  /* Limiar adaptativo dinâmico */
    float    last_onset_time;
    float    time_accum;
    float    recent_intervals[MUSIC_HISTORY_LEN];
    int      interval_count;
    int      interval_idx;

    /* Handles de captura de áudio (microfone / desktop loopback) */
    void    *mic_handle;
    void    *desktop_handle;
    uint32_t sample_rate;
    float    media_player_confidence; /* Detectado via player de mídia / janelas de áudio */
} SenseMusic;

/* Inicializa o subsistema de reconhecimento musical e ritmo */
bool  sense_music_init(SenseMusic *m, uint32_t sample_rate);

/* Atualiza análise de áudio de microfone, desktop e títulos de mídia ativa */
void  sense_music_update(SenseMusic *m, float dt, const char *window_title);

/* Consultas públicas */
float sense_music_get_bpm(const SenseMusic *m);
float sense_music_get_phase(const SenseMusic *m);
bool  sense_music_is_on_beat(const SenseMusic *m);
bool  sense_music_is_playing(const SenseMusic *m);
float sense_music_get_energy(const SenseMusic *m);

/* Finaliza streams e libera recursos */
void  sense_music_destroy(SenseMusic *m);

#endif /* PET_SENSE_MUSIC_H */
