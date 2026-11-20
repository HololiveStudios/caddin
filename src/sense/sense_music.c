#include "sense_music.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#if defined(__linux__) && !defined(WITHOUT_ALSA)
#include <alsa/asoundlib.h>
#elif defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Lista de assinaturas conhecidas de tocadores de música e streaming no desktop */
static const char *MEDIA_SIGNATURES[] = {
    "youtube", "spotify", "soundcloud", "deezer", "apple music",
    "vlc", "rhythmbox", "audacious", "clementine", "mpv",
    "foobar2000", "aimp", "winamp", "music", "radio", "bandcamp"
};
#define NUM_MEDIA_SIGNATURES ((int)(sizeof(MEDIA_SIGNATURES) / sizeof(MEDIA_SIGNATURES[0])))

/* Função utilitária para buscar substrings insensíveis a maiúsculas/minúsculas */
static bool title_contains_media(const char *title) {
    if (!title || title[0] == '\0') return false;
    char lower[256];
    size_t len = strlen(title);
    if (len >= sizeof(lower)) len = sizeof(lower) - 1;
    for (size_t i = 0; i < len; ++i) {
        lower[i] = (char)tolower((unsigned char)title[i]);
    }
    lower[len] = '\0';

    for (int i = 0; i < NUM_MEDIA_SIGNATURES; ++i) {
        if (strstr(lower, MEDIA_SIGNATURES[i]) != NULL) {
            return true;
        }
    }
    return false;
}

bool sense_music_init(SenseMusic *m, uint32_t sample_rate) {
    if (!m) return false;
    memset(m, 0, sizeof(*m));
    m->sample_rate = (sample_rate > 0) ? sample_rate : 16000;
    m->bpm = 120.0f;
    m->beat_interval = 0.50f;
    m->energy_threshold = 0.08f;
    m->available = false;

#if defined(__linux__) && !defined(WITHOUT_ALSA)
    snd_pcm_t *pcm_mic = NULL;
    /* Tenta abrir captura padrão (microfone / desktop default) */
    if (snd_pcm_open(&pcm_mic, "default", SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK) >= 0 && pcm_mic) {
        snd_pcm_hw_params_t *params = NULL;
        snd_pcm_hw_params_alloca(&params);
        snd_pcm_hw_params_any(pcm_mic, params);
        snd_pcm_hw_params_set_access(pcm_mic, params, SND_PCM_ACCESS_RW_INTERLEAVED);
        snd_pcm_hw_params_set_format(pcm_mic, params, SND_PCM_FORMAT_S16_LE);
        snd_pcm_hw_params_set_channels(pcm_mic, params, 1);
        unsigned int rate = m->sample_rate;
        snd_pcm_hw_params_set_rate_near(pcm_mic, params, &rate, 0);
        snd_pcm_uframes_t frames = 512;
        snd_pcm_hw_params_set_period_size_near(pcm_mic, params, &frames, 0);

        if (snd_pcm_hw_params(pcm_mic, params) >= 0) {
            snd_pcm_prepare(pcm_mic);
            snd_pcm_start(pcm_mic);
            m->mic_handle = (void *)pcm_mic;
            m->available = true;
        } else {
            snd_pcm_close(pcm_mic);
        }
    }

    /* Tenta abrir um segundo stream de loopback/monitor do PulseAudio/PipeWire se disponível */
    snd_pcm_t *pcm_pulse = NULL;
    if (snd_pcm_open(&pcm_pulse, "pulse", SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK) >= 0 && pcm_pulse) {
        snd_pcm_hw_params_t *params = NULL;
        snd_pcm_hw_params_alloca(&params);
        snd_pcm_hw_params_any(pcm_pulse, params);
        snd_pcm_hw_params_set_access(pcm_pulse, params, SND_PCM_ACCESS_RW_INTERLEAVED);
        snd_pcm_hw_params_set_format(pcm_pulse, params, SND_PCM_FORMAT_S16_LE);
        snd_pcm_hw_params_set_channels(pcm_pulse, params, 1);
        unsigned int rate = m->sample_rate;
        snd_pcm_hw_params_set_rate_near(pcm_pulse, params, &rate, 0);
        snd_pcm_uframes_t frames = 512;
        snd_pcm_hw_params_set_period_size_near(pcm_pulse, params, &frames, 0);

        if (snd_pcm_hw_params(pcm_pulse, params) >= 0) {
            snd_pcm_prepare(pcm_pulse);
            snd_pcm_start(pcm_pulse);
            m->desktop_handle = (void *)pcm_pulse;
            m->available = true;
        } else {
            snd_pcm_close(pcm_pulse);
        }
    }
#endif

    return true;
}

void sense_music_update(SenseMusic *m, float dt, const char *window_title) {
    if (!m) return;

    m->time_accum += dt;
    m->is_on_beat = false;

    /* 1. Verificação contextual de títulos de janelas de tocadores no Desktop */
    if (title_contains_media(window_title)) {
        m->media_player_confidence = (m->media_player_confidence < 0.95f) ?
                                      m->media_player_confidence + dt * 0.5f : 1.0f;
    } else {
        m->media_player_confidence = (m->media_player_confidence > 0.0f) ?
                                      m->media_player_confidence - dt * 0.1f : 0.0f;
    }

    /* 2. Leitura de amostras de áudio (microfone ou desktop loopback) */
    int16_t buffer[512];
    long frames_read = 0;

#if defined(__linux__) && !defined(WITHOUT_ALSA)
    /* Prioriza desktop loopback se tiver amostras, senão lê do microfone */
    if (m->desktop_handle) {
        frames_read = snd_pcm_readi((snd_pcm_t *)m->desktop_handle, buffer, 512);
        if (frames_read == -EAGAIN) frames_read = 0;
        else if (frames_read < 0) {
            snd_pcm_recover((snd_pcm_t *)m->desktop_handle, (int)frames_read, 0);
            frames_read = 0;
        }
    }
    if (frames_read <= 0 && m->mic_handle) {
        frames_read = snd_pcm_readi((snd_pcm_t *)m->mic_handle, buffer, 512);
        if (frames_read == -EAGAIN) frames_read = 0;
        else if (frames_read < 0) {
            snd_pcm_recover((snd_pcm_t *)m->mic_handle, (int)frames_read, 0);
            frames_read = 0;
        }
    }
#endif

    float current_energy = 0.0f;
    if (frames_read > 0) {
        double sum_sq = 0.0;
        for (long i = 0; i < frames_read; ++i) {
            double s = (double)buffer[i] / 32768.0;
            sum_sq += s * s;
        }
        current_energy = (float)sqrt(sum_sq / (double)frames_read);
    }

    /* Filtro IIR suave para energia geral */
    m->music_energy = m->music_energy * 0.75f + current_energy * 0.25f;

    /* 3. Detecção de Onsets (Ataques rítmicos de bumbo/batida) */
    float energy_diff = current_energy - m->prev_energy;
    if (energy_diff < 0.0f) energy_diff = 0.0f;
    m->prev_energy = current_energy;

    /* Limiar adaptativo dinâmico */
    m->energy_threshold = m->energy_threshold * 0.96f + (m->music_energy * 1.35f + 0.03f) * 0.04f;

    /* Se houver pico de energia acima do limiar e espaçado em pelo menos 220ms (limite de 270 BPM) */
    float time_since_last_onset = m->time_accum - m->last_onset_time;
    if (energy_diff > m->energy_threshold && time_since_last_onset > 0.22f) {
        /* Registra o intervalo inter-batidas (IBI) */
        if (time_since_last_onset < 1.25f) { /* Menos de 1.25s = mais de 48 BPM */
            m->recent_intervals[m->interval_idx] = time_since_last_onset;
            m->interval_idx = (m->interval_idx + 1) % MUSIC_HISTORY_LEN;
            if (m->interval_count < MUSIC_HISTORY_LEN) m->interval_count++;

            /* Calcula regularidade dos intervalos (teste de cadência musical) */
            if (m->interval_count >= 4) {
                float sum_intervals = 0.0f;
                for (int i = 0; i < m->interval_count; ++i) {
                    sum_intervals += m->recent_intervals[i];
                }
                float avg_interval = sum_intervals / (float)m->interval_count;

                /* Desvio padrão */
                float variance = 0.0f;
                for (int i = 0; i < m->interval_count; ++i) {
                    float diff = m->recent_intervals[i] - avg_interval;
                    variance += diff * diff;
                }
                float std_dev = sqrtf(variance / (float)m->interval_count);

                /* Coeficiente de variação < 22% indica ritmo musical regular! */
                if (std_dev / avg_interval < 0.22f && avg_interval >= 0.30f && avg_interval <= 1.0f) {
                    m->beat_interval = avg_interval;
                    m->bpm = 60.0f / avg_interval;
                    m->music_confidence = (m->music_confidence < 0.95f) ?
                                           m->music_confidence + 0.15f : 1.0f;
                }
            }
        }
        m->last_onset_time = m->time_accum;
    } else {
        /* Decaimento passivo da confiança de ritmo */
        m->music_confidence *= (1.0f - dt * 0.18f);
    }

    /* Combinação com detecção de mídia no desktop */
    float total_confidence = m->music_confidence + m->media_player_confidence * 0.5f;
    if (total_confidence > 1.0f) total_confidence = 1.0f;

    /* Confirmação de música: confiança >= 0.35 E energia perceptível (ou player ativo) */
    if (total_confidence >= 0.35f && (m->music_energy > 0.04f || m->media_player_confidence > 0.4f)) {
        m->is_music = true;
    } else {
        m->is_music = false;
    }

    /* 4. Gerador de Fase Rítmica (PLL sincronizado com BPM) */
    if (m->beat_interval < 0.25f) m->beat_interval = 0.50f; /* Fallback 120 BPM */
    m->beat_timer += dt;
    if (m->beat_timer >= m->beat_interval) {
        m->beat_timer -= m->beat_interval;
        m->is_on_beat = true;
        m->beat_intensity = (m->music_energy > 0.1f) ? m->music_energy * 2.5f : 0.6f;
        if (m->beat_intensity > 1.0f) m->beat_intensity = 1.0f;
    }

    m->beat_phase = m->beat_timer / m->beat_interval;
    if (m->beat_phase > 1.0f) m->beat_phase = 1.0f;
}

float sense_music_get_bpm(const SenseMusic *m) {
    return m ? m->bpm : 120.0f;
}

float sense_music_get_phase(const SenseMusic *m) {
    return m ? m->beat_phase : 0.0f;
}

bool sense_music_is_on_beat(const SenseMusic *m) {
    return m ? m->is_on_beat : false;
}

bool sense_music_is_playing(const SenseMusic *m) {
    return m ? m->is_music : false;
}

float sense_music_get_energy(const SenseMusic *m) {
    return m ? m->music_energy : 0.0f;
}

void sense_music_destroy(SenseMusic *m) {
    if (!m) return;
#if defined(__linux__) && !defined(WITHOUT_ALSA)
    if (m->mic_handle) {
        snd_pcm_close((snd_pcm_t *)m->mic_handle);
        m->mic_handle = NULL;
    }
    if (m->desktop_handle) {
        snd_pcm_close((snd_pcm_t *)m->desktop_handle);
        m->desktop_handle = NULL;
    }
#endif
    m->available = false;
}
