#include "voice_synth.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void voice_synth_init(VoiceSynth *vs, uint32_t sample_rate) {
    if (!vs) return;
    vs->current_type = VOICE_IDLE;
    vs->sample_rate = (sample_rate > 0) ? sample_rate : 16000;
    vs->sample_index = 0;
    vs->total_samples = 0;
    vs->f0 = 440.0f;
    vs->f1 = 880.0f;
    vs->phase = 0.0f;
    vs->mod_phase = 0.0f;
}

void voice_synth_trigger(VoiceSynth *vs, VoiceType type, float duration_sec) {
    if (!vs) return;
    vs->current_type = type;
    vs->sample_index = 0;
    vs->total_samples = (uint32_t)(duration_sec * (float)vs->sample_rate);
    vs->phase = 0.0f;
    vs->mod_phase = 0.0f;

    switch (type) {
        case VOICE_PURR:
            vs->f0 = 68.0f;  /* Ronronar orgânico grave */
            vs->f1 = 18.0f;  /* Modulação rítmica da garganta */
            break;
        case VOICE_CHIRP:
            vs->f0 = 480.0f; /* Início do pio curioso */
            vs->f1 = 820.0f; /* Final ascendente fofo */
            break;
        case VOICE_ALARM:
            vs->f0 = 580.0f; /* Guincho de protesto felino (não estridente) */
            vs->f1 = 360.0f; /* Queda de tom expressiva */
            break;
        case VOICE_FORMANT:
            vs->f0 = 260.0f; /* Frequência vocal fundamental de balbucio */
            vs->f1 = 650.0f; /* Formante suave */
            break;
        default:
            vs->total_samples = 0;
            break;
    }
}

size_t voice_synth_render(VoiceSynth *vs, int16_t *out_buffer, size_t max_samples) {
    if (!vs || !out_buffer || vs->current_type == VOICE_IDLE) return 0;

    size_t count = 0;
    float dt = 1.0f / (float)vs->sample_rate;

    while (count < max_samples && vs->sample_index < vs->total_samples) {
        float t = (float)vs->sample_index / (float)vs->total_samples; /* [0.0 .. 1.0] */
        float env = 1.0f;

        /* Envelope ADSR simples (fade in / fade out para evitar clicks) */
        if (t < 0.1f) {
            env = t / 0.1f;
        } else if (t > 0.8f) {
            env = (1.0f - t) / 0.2f;
        }

        float sample = 0.0f;

        float gain = 14000.0f;

        if (vs->current_type == VOICE_PURR) {
            /* Modulação de amplitude com frequência baixa */
            float carrier = sinf(vs->phase);
            float lfo = 0.5f + 0.5f * sinf(vs->mod_phase);
            sample = carrier * lfo;
            gain = 11000.0f;
            vs->phase += 2.0f * (float)M_PI * vs->f0 * dt;
            vs->mod_phase += 2.0f * (float)M_PI * vs->f1 * dt;
        } else if (vs->current_type == VOICE_CHIRP) {
            /* Varredura linear ascendente de frequência */
            float current_f = vs->f0 + (vs->f1 - vs->f0) * t;
            sample = sinf(vs->phase);
            gain = 12000.0f;
            vs->phase += 2.0f * (float)M_PI * current_f * dt;
        } else if (vs->current_type == VOICE_ALARM) {
            /* Queda de frequência expressiva com leve saturação (miado assustado) */
            float current_f = vs->f0 + (vs->f1 - vs->f0) * t;
            float s1 = sinf(vs->phase);
            float s2 = sinf(vs->phase * 2.0f);
            sample = 0.7f * s1 + 0.3f * s2;
            gain = 15000.0f;
            vs->phase += 2.0f * (float)M_PI * current_f * dt;
        } else if (vs->current_type == VOICE_FORMANT) {
            /* Simulação de trato vocal de balbucio */
            float glottal = sinf(vs->phase);
            float formant = sinf(vs->mod_phase);
            sample = glottal * 0.4f + formant * 0.6f;
            gain = 10000.0f;
            vs->phase += 2.0f * (float)M_PI * vs->f0 * dt;
            vs->mod_phase += 2.0f * (float)M_PI * vs->f1 * dt;
        }

        /* Normalização para int16_t */
        float scaled = sample * env * gain;
        if (scaled > 32767.0f) scaled = 32767.0f;
        if (scaled < -32768.0f) scaled = -32768.0f;

        out_buffer[count++] = (int16_t)scaled;
        vs->sample_index++;
    }

    if (vs->sample_index >= vs->total_samples) {
        vs->current_type = VOICE_IDLE;
    }

    return count;
}
