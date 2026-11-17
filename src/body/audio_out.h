/* audio_out.h — saída de áudio PCM via ALSA (Linux) / WinMM (Windows)
 * Zero alocações em tempo real. Buffer de anel pré-alocado (4096 amostras).
 */
#ifndef AUDIO_OUT_H
#define AUDIO_OUT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Inicializa o dispositivo de saída de áudio.
 * sample_rate: taxa de amostragem em Hz (ex: 16000)
 * Retorna true em sucesso. Em caso de falha, write() é no-op seguro. */
bool audio_out_init(int sample_rate);

/* Enfileira amostras PCM s16le para playback não-bloqueante.
 * Pode ser chamado de qualquer thread.
 * n: número de amostras (não bytes). */
void audio_out_write(const int16_t *buf, size_t n);

/* Finaliza e fecha o dispositivo de áudio. */
void audio_out_close(void);

/* Retorna true se o dispositivo foi inicializado com sucesso. */
bool audio_out_is_open(void);

#endif /* AUDIO_OUT_H */
