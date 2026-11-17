#include "sense_mic.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#if defined(__linux__) && !defined(WITHOUT_ALSA)
#include <alsa/asoundlib.h>
#elif defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#endif

bool sense_mic_init(SenseMic *mic, uint32_t sample_rate) {
    if (!mic) return false;
    memset(mic, 0, sizeof(*mic));
    mic->sample_rate = (sample_rate > 0) ? sample_rate : 16000;
    mic->available = false;
    mic->enabled = true;

#if defined(__linux__) && !defined(WITHOUT_ALSA)
    snd_pcm_t *pcm_capture = NULL;
    /* Tenta abrir ALSA em modo de captura não-bloqueante */
    int err = snd_pcm_open(&pcm_capture, "default", SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
    if (err >= 0 && pcm_capture) {
        snd_pcm_hw_params_t *params = NULL;
        snd_pcm_hw_params_alloca(&params);
        snd_pcm_hw_params_any(pcm_capture, params);
        snd_pcm_hw_params_set_access(pcm_capture, params, SND_PCM_ACCESS_RW_INTERLEAVED);
        snd_pcm_hw_params_set_format(pcm_capture, params, SND_PCM_FORMAT_S16_LE);
        snd_pcm_hw_params_set_channels(pcm_capture, params, 1);
        unsigned int rate = mic->sample_rate;
        snd_pcm_hw_params_set_rate_near(pcm_capture, params, &rate, 0);
        snd_pcm_uframes_t frames = 512;
        snd_pcm_hw_params_set_period_size_near(pcm_capture, params, &frames, 0);

        if (snd_pcm_hw_params(pcm_capture, params) >= 0) {
            snd_pcm_prepare(pcm_capture);
            snd_pcm_start(pcm_capture);
            mic->backend_handle = (void *)pcm_capture;
            mic->available = true;
        } else {
            snd_pcm_close(pcm_capture);
        }
    }
#endif

    return mic->available;
}

void sense_mic_update(SenseMic *mic, float dt) {
    if (!mic || !mic->available || !mic->enabled) {
        if (mic) {
            mic->current_rms *= (1.0f - dt * 5.0f);
            mic->clapped = false;
        }
        return;
    }

    int16_t buffer[512];
    long frames_read = 0;

#if defined(__linux__) && !defined(WITHOUT_ALSA)
    if (mic->backend_handle) {
        snd_pcm_t *pcm = (snd_pcm_t *)mic->backend_handle;
        frames_read = snd_pcm_readi(pcm, buffer, 512);
        if (frames_read == -EAGAIN) {
            frames_read = 0;
        } else if (frames_read < 0) {
            snd_pcm_recover(pcm, (int)frames_read, 0);
            frames_read = 0;
        }
    }
#endif

    if (frames_read > 0) {
        double sum_sq = 0.0;
        for (long i = 0; i < frames_read; ++i) {
            double s = (double)buffer[i] / 32768.0;
            sum_sq += s * s;
        }
        float instant_rms = (float)sqrt(sum_sq / (double)frames_read);

        /* Filtro IIR suave para RMS */
        mic->current_rms = mic->current_rms * 0.70f + instant_rms * 0.30f;

        /* Detecção de palmas: subida abrupta acima de 0.35 */
        if (instant_rms > 0.35f && (instant_rms - mic->current_rms) > 0.15f) {
            mic->clapped = true;
        } else {
            mic->clapped = false;
        }

        if (mic->current_rms > mic->peak_rms) {
            mic->peak_rms = mic->current_rms;
        } else {
            mic->peak_rms *= (1.0f - dt * 2.0f);
        }
    } else {
        mic->current_rms *= (1.0f - dt * 6.0f);
        mic->clapped = false;
    }
}

float sense_mic_get_rms(const SenseMic *mic) {
    return mic ? mic->current_rms : 0.0f;
}

bool sense_mic_is_loud(const SenseMic *mic) {
    return mic ? (mic->clapped || mic->current_rms > 0.50f) : false;
}

void sense_mic_destroy(SenseMic *mic) {
    if (!mic) return;
#if defined(__linux__) && !defined(WITHOUT_ALSA)
    if (mic->backend_handle) {
        snd_pcm_close((snd_pcm_t *)mic->backend_handle);
        mic->backend_handle = NULL;
    }
#endif
    mic->available = false;
}
