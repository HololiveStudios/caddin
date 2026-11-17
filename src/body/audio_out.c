/* audio_out.c — saída PCM não-bloqueante
 * Linux: ALSA snd_pcm direto (sem dependência de lib de alto nível)
 * Windows: WinMM waveOut
 * Fallback silencioso se o dispositivo não puder ser aberto.
 */
#define _POSIX_C_SOURCE 200809L
#include "audio_out.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ═══════════════════════════════════════════════════════════
 * Ring buffer compartilhado (lock-free single-producer / consumer)
 * ═══════════════════════════════════════════════════════════ */
#define RING_SIZE   8192   /* amostras PCM s16 */
static int16_t  g_ring[RING_SIZE];
static volatile size_t g_head = 0;   /* produtor (main thread) */
static volatile size_t g_tail = 0;   /* consumidor (audio thread / callback) */

static bool g_open = false;
static int  g_rate = 16000;

/* ═══════════════════════════════════════════════════════════
 * IMPLEMENTAÇÃO LINUX (ALSA)
 * ═══════════════════════════════════════════════════════════ */
#if defined(__linux__) && !defined(_WIN32)

#include <alsa/asoundlib.h>
#include <alloca.h>
#include <pthread.h>

static snd_pcm_t *g_pcm = NULL;
static pthread_t  g_thread;
static volatile int g_thread_run = 0;

static void *audio_thread(void *arg) {
    (void)arg;
    int16_t silence[256];
    memset(silence, 0, sizeof(silence));
    snd_pcm_sframes_t avail;

    while (g_thread_run) {
        avail = snd_pcm_avail_update(g_pcm);
        if (avail < 0) {
            snd_pcm_recover(g_pcm, (int)avail, 1);
            continue;
        }
        if (avail == 0) {
            snd_pcm_wait(g_pcm, 20);
            continue;
        }

        /* Drena ring buffer para a placa de som */
        size_t head = g_head;
        size_t tail = g_tail;
        size_t pending = (head - tail) & (RING_SIZE * 2 - 1);
        /* Garante apenas até RING_SIZE */
        if (pending > RING_SIZE) pending = RING_SIZE;

        if (pending == 0) {
            /* Silêncio para não deixar ALSA estourar o underrun */
            size_t frames = (size_t)avail;
            if (frames > 256) frames = 256;
            snd_pcm_writei(g_pcm, silence, (snd_pcm_uframes_t)frames);
        } else {
            size_t frames = pending;
            if (frames > (size_t)avail) frames = (size_t)avail;

            /* Cópia segura evitando wrap-around do ring */
            size_t idx = tail % RING_SIZE;
            size_t chunk1 = RING_SIZE - idx;
            if (chunk1 > frames) chunk1 = frames;
            size_t chunk2 = frames - chunk1;

            int16_t tmp[512];
            if (frames > 512) frames = 512;
            chunk1 = (frames < (RING_SIZE - idx)) ? frames : (RING_SIZE - idx);
            chunk2 = frames - chunk1;
            memcpy(tmp,          &g_ring[idx],  chunk1 * sizeof(int16_t));
            if (chunk2 > 0)
                memcpy(tmp + chunk1, &g_ring[0],  chunk2 * sizeof(int16_t));

            snd_pcm_sframes_t wrote = snd_pcm_writei(g_pcm, tmp, (snd_pcm_uframes_t)frames);
            if (wrote == -EPIPE) {
                snd_pcm_recover(g_pcm, (int)wrote, 1);
            } else if (wrote > 0) {
                g_tail = tail + (size_t)wrote;
            }
        }
    }
    return NULL;
}

bool audio_out_init(int sample_rate) {
    g_rate = sample_rate;
    int err;

    err = snd_pcm_open(&g_pcm, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
    if (err < 0) {
        /* Tenta "pulse" como fallback (PulseAudio plugin ALSA) */
        err = snd_pcm_open(&g_pcm, "pulse", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
    }
    if (err < 0) {
        fprintf(stderr, "[AUDIO] Falha ao abrir dispositivo ALSA: %s\n", snd_strerror(err));
        g_pcm = NULL;
        return false;
    }

    snd_pcm_hw_params_t *hw;
    snd_pcm_hw_params_alloca(&hw);
    snd_pcm_hw_params_any(g_pcm, hw);
    snd_pcm_hw_params_set_access(g_pcm, hw, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(g_pcm, hw, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(g_pcm, hw, 1);  /* mono */
    unsigned int rate = (unsigned int)sample_rate;
    snd_pcm_hw_params_set_rate_near(g_pcm, hw, &rate, 0);
    snd_pcm_uframes_t period = 512;
    snd_pcm_hw_params_set_period_size_near(g_pcm, hw, &period, 0);
    snd_pcm_uframes_t buf_size = 4096;
    snd_pcm_hw_params_set_buffer_size_near(g_pcm, hw, &buf_size);

    err = snd_pcm_hw_params(g_pcm, hw);
    if (err < 0) {
        fprintf(stderr, "[AUDIO] Falha ao configurar HW ALSA: %s\n", snd_strerror(err));
        snd_pcm_close(g_pcm);
        g_pcm = NULL;
        return false;
    }

    snd_pcm_prepare(g_pcm);

    g_head = g_tail = 0;
    g_thread_run = 1;
    if (pthread_create(&g_thread, NULL, audio_thread, NULL) != 0) {
        fprintf(stderr, "[AUDIO] Falha ao criar thread de áudio.\n");
        snd_pcm_close(g_pcm);
        g_pcm = NULL;
        return false;
    }

    g_open = true;
    printf("[AUDIO] ALSA inicializado: %d Hz, mono, s16le\n", sample_rate);
    return true;
}

void audio_out_write(const int16_t *buf, size_t n) {
    if (!g_open || !buf || n == 0) return;
    for (size_t i = 0; i < n; i++) {
        size_t next_head = g_head + 1;
        /* Se o ring estiver cheio, descarta (non-blocking) */
        if ((next_head % RING_SIZE) == (g_tail % RING_SIZE)) return;
        g_ring[g_head % RING_SIZE] = buf[i];
        g_head = next_head;
    }
}

void audio_out_close(void) {
    if (!g_open) return;
    g_thread_run = 0;
    pthread_join(g_thread, NULL);
    if (g_pcm) {
        snd_pcm_drain(g_pcm);
        snd_pcm_close(g_pcm);
        g_pcm = NULL;
    }
    g_open = false;
}

bool audio_out_is_open(void) { return g_open; }

/* ═══════════════════════════════════════════════════════════
 * IMPLEMENTAÇÃO WINDOWS (WinMM waveOut)
 * ═══════════════════════════════════════════════════════════ */
#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#define WAVEBUF_COUNT  4
#define WAVEBUF_FRAMES 512

static HWAVEOUT g_hwave = NULL;
static WAVEHDR  g_whdr[WAVEBUF_COUNT];
static int16_t  g_wbuf[WAVEBUF_COUNT][WAVEBUF_FRAMES];
static int      g_buf_idx = 0;

static void CALLBACK wave_callback(HWAVEOUT hwo, UINT msg,
                                   DWORD_PTR inst, DWORD_PTR p1, DWORD_PTR p2) {
    (void)hwo; (void)inst; (void)p2;
    if (msg != WOM_DONE) return;
    WAVEHDR *hdr = (WAVEHDR *)p1;
    /* Preenche com dados do ring ou silêncio */
    int16_t *dst = (int16_t *)hdr->lpData;
    size_t frames = WAVEBUF_FRAMES;
    size_t head = g_head, tail = g_tail;
    size_t avail = (head - tail) % RING_SIZE;
    if (avail > frames) avail = frames;
    for (size_t i = 0; i < avail; i++) {
        dst[i] = g_ring[(tail + i) % RING_SIZE];
    }
    for (size_t i = avail; i < frames; i++) {
        dst[i] = 0;
    }
    g_tail = tail + avail;
    hdr->dwBufferLength = (DWORD)(frames * sizeof(int16_t));
    waveOutPrepareHeader(g_hwave, hdr, sizeof(WAVEHDR));
    waveOutWrite(g_hwave, hdr, sizeof(WAVEHDR));
}

bool audio_out_init(int sample_rate) {
    g_rate = sample_rate;
    WAVEFORMATEX wfx = {0};
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = 1;
    wfx.nSamplesPerSec  = (DWORD)sample_rate;
    wfx.wBitsPerSample  = 16;
    wfx.nBlockAlign     = 2;
    wfx.nAvgBytesPerSec = (DWORD)(sample_rate * 2);

    MMRESULT res = waveOutOpen(&g_hwave, WAVE_MAPPER, &wfx,
                               (DWORD_PTR)wave_callback, 0,
                               CALLBACK_FUNCTION);
    if (res != MMSYSERR_NOERROR) {
        fprintf(stderr, "[AUDIO] waveOutOpen falhou: %u\n", res);
        g_hwave = NULL;
        return false;
    }

    g_head = g_tail = 0;
    memset(g_whdr, 0, sizeof(g_whdr));
    for (int i = 0; i < WAVEBUF_COUNT; i++) {
        g_whdr[i].lpData         = (LPSTR)g_wbuf[i];
        g_whdr[i].dwBufferLength = WAVEBUF_FRAMES * sizeof(int16_t);
        waveOutPrepareHeader(g_hwave, &g_whdr[i], sizeof(WAVEHDR));
        memset(g_wbuf[i], 0, WAVEBUF_FRAMES * sizeof(int16_t));
        waveOutWrite(g_hwave, &g_whdr[i], sizeof(WAVEHDR));
    }

    g_open = true;
    printf("[AUDIO] WinMM waveOut inicializado: %d Hz, mono, s16le\n", sample_rate);
    return true;
}

void audio_out_write(const int16_t *buf, size_t n) {
    if (!g_open || !buf || n == 0) return;
    for (size_t i = 0; i < n; i++) {
        size_t next_head = g_head + 1;
        if ((next_head % RING_SIZE) == (g_tail % RING_SIZE)) return;
        g_ring[g_head % RING_SIZE] = buf[i];
        g_head = next_head;
    }
}

void audio_out_close(void) {
    if (!g_open || !g_hwave) return;
    waveOutReset(g_hwave);
    for (int i = 0; i < WAVEBUF_COUNT; i++) {
        waveOutUnprepareHeader(g_hwave, &g_whdr[i], sizeof(WAVEHDR));
    }
    waveOutClose(g_hwave);
    g_hwave = NULL;
    g_open = false;
}

bool audio_out_is_open(void) { return g_open; }

/* ═══════════════════════════════════════════════════════════
 * STUB — outros sistemas (macOS, etc.)
 * ═══════════════════════════════════════════════════════════ */
#else

bool audio_out_init(int sample_rate) {
    (void)sample_rate;
    fprintf(stderr, "[AUDIO] Plataforma sem suporte de áudio.\n");
    return false;
}
void audio_out_write(const int16_t *buf, size_t n) { (void)buf; (void)n; }
void audio_out_close(void) {}
bool audio_out_is_open(void) { return false; }

#endif /* platform */
