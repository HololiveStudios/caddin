#ifndef PET_BODY_VOICE_SYNTH_H
#define PET_BODY_VOICE_SYNTH_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    VOICE_IDLE = 0,
    VOICE_PURR,
    VOICE_CHIRP,
    VOICE_ALARM,
    VOICE_FORMANT
} VoiceType;

typedef struct {
    VoiceType current_type;
    uint32_t  sample_rate;
    uint32_t  sample_index;
    uint32_t  total_samples;
    float     f0;
    float     f1;
    float     phase;
    float     mod_phase;
} VoiceSynth;

void voice_synth_init(VoiceSynth *vs, uint32_t sample_rate);
void voice_synth_trigger(VoiceSynth *vs, VoiceType type, float duration_sec);
/* Gera amostras PCM 16-bit mono no buffer de saída. Retorna quantidade de amostras geradas. */
size_t voice_synth_render(VoiceSynth *vs, int16_t *out_buffer, size_t max_samples);

#endif /* PET_BODY_VOICE_SYNTH_H */
