/*
 * voice_synth.c — Sintetizador Procedural de Voz Emocional
 *
 * Arquitetura: síntese puramente em tempo real, zero malloc, todas as fases
 * armazenadas na struct VoiceSynth.  11 tipos de voz emocional inspirados
 * biologicamente em vocalizações felinas/mamíferas.
 *
 * Taxa de amostragem: 16000 Hz, mono, s16le.
 */

#include "voice_synth.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TWO_PI (2.0f * (float)M_PI)

/* ---------------------------------------------------------------------------
 * Utilitários internos
 * --------------------------------------------------------------------------- */

/* Pseudo-ruído branco xorshift32 → [-1.0, 1.0] */
static inline float noise_next(uint32_t *seed) {
    uint32_t x = *seed;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *seed = x;
    /* mapeia uint32 para [-1.0, 1.0] */
    return (float)(int32_t)x * (1.0f / 2147483648.0f);
}

/* Clamp float para int16 sem UB */
static inline int16_t f_to_i16(float v) {
    if (v >  32767.0f) return  32767;
    if (v < -32768.0f) return -32768;
    return (int16_t)v;
}

/* Envelope ADSR simples: fade-in nos primeiros att%, fade-out nos últimos rel% */
static inline float envelope(float t, float att, float rel) {
    if (t < att)        return t / att;
    if (t > (1.0f - rel)) return (1.0f - t) / rel;
    return 1.0f;
}

/* ---------------------------------------------------------------------------
 * Inicialização
 * --------------------------------------------------------------------------- */
void voice_synth_init(VoiceSynth *vs, uint32_t sample_rate) {
    if (!vs) return;
    memset(vs, 0, sizeof(*vs));
    vs->current_type = VOICE_IDLE;
    vs->sample_rate  = (sample_rate > 0) ? sample_rate : 16000;
    vs->noise_seed   = 0xDEADBEEFu;
    vs->intensity    = 0.5f;
    vs->pitch_scale  = 1.0f;
}

void voice_synth_set_maturity_stage(VoiceSynth *vs, int stage) {
    if (!vs) return;
    switch (stage) {
    case 0: /* ONTO_FILHOTE */  vs->pitch_scale = 1.35f; break;
    case 1: /* ONTO_JUVENIL */  vs->pitch_scale = 1.15f; break;
    case 2: /* ONTO_ADULTO */   vs->pitch_scale = 1.00f; break;
    case 3: /* ONTO_VETERANO */ vs->pitch_scale = 0.88f; break;
    default: vs->pitch_scale = 1.00f; break;
    }
}

/* ---------------------------------------------------------------------------
 * voice_synth_trigger_ex — configura todos os parâmetros por tipo
 * --------------------------------------------------------------------------- */
void voice_synth_trigger_ex(VoiceSynth *vs, VoiceType type,
                             float duration_sec, float intensity) {
    if (!vs || type == VOICE_IDLE || type >= VOICE_TYPE_COUNT) return;

    /* Se já está tocando o mesmo tipo, não reinicia (evita corte) */
    if (vs->current_type == type && vs->sample_index < vs->total_samples) return;

    /* Zera fases para evitar clique na retomada */
    vs->current_type  = type;
    vs->sample_index  = 0;
    vs->total_samples = (uint32_t)(duration_sec * (float)vs->sample_rate);
    vs->phase         = 0.0f;
    vs->mod_phase     = 0.0f;
    vs->phase2        = 0.0f;
    vs->vibrato_phase = 0.0f;
    vs->tremolo_phase = 0.0f;
    vs->breath_phase  = 0.0f;
    vs->repeat_count  = 0;
    vs->intensity     = (intensity < 0.0f) ? 0.0f : (intensity > 1.0f) ? 1.0f : intensity;

    uint32_t sr = vs->sample_rate;

    switch (type) {

    /* ── PURR_CONTENT: ronron de carinho moderado ───────────────────────── */
    case VOICE_PURR_CONTENT:
        vs->f0           = 68.0f;      /* fundamental grave de garganta */
        vs->f1           = 20.0f;      /* LFO AM: ritmo de inhale/exhale */
        vs->f2           = 136.0f;     /* 1º harmônico (oitava) */
        vs->vibrato_rate  = 0.0f;      /* sem vibrato no conteúdo relaxado */
        vs->vibrato_depth = 0.0f;
        vs->tremolo_rate  = 20.0f;     /* AM 20 Hz = rítmica de ronron */
        vs->repeat_max   = 0;
        break;

    /* ── PURR_DEEP: ronron profundo de êxtase ───────────────────────────── */
    case VOICE_PURR_DEEP:
        vs->f0           = 55.0f;      /* mais grave, peito vibrando */
        vs->f1           = 24.0f;      /* AM mais rápida */
        vs->f2           = 110.0f;     /* harmônico forte */
        vs->vibrato_rate  = 2.5f;      /* leve ondulação de êxtase */
        vs->vibrato_depth = 3.0f;      /* ±3 Hz */
        vs->tremolo_rate  = 24.0f;
        vs->repeat_max   = 0;
        break;

    /* ── TRILL: trilo de alegria/brincadeira (3 chirps rápidos) ─────────── */
    case VOICE_TRILL:
        vs->f0           = 450.0f;    /* início do pio */
        vs->f1           = 780.0f;    /* pico ascendente */
        vs->repeat_max   = 3;         /* 3 piados */
        vs->repeat_len   = (uint32_t)(0.065f * (float)sr); /* 65ms por piado */
        vs->gap_len      = (uint32_t)(0.040f * (float)sr); /* 40ms de pausa */
        /* total_samples para acomodar as repetições + gaps */
        vs->total_samples = vs->repeat_max * (vs->repeat_len + vs->gap_len);
        vs->vibrato_rate  = 0.0f;
        vs->vibrato_depth = 0.0f;
        break;

    /* ── CHIRP_HAPPY: 2 notas curtas de felicidade ──────────────────────── */
    case VOICE_CHIRP_HAPPY:
        vs->f0           = 500.0f;    /* nota 1 */
        vs->f1           = 660.0f;    /* nota 2 (maior = mais alegre) */
        vs->vibrato_rate  = 0.0f;
        vs->vibrato_depth = 0.0f;
        vs->repeat_max   = 2;
        vs->repeat_len   = (uint32_t)(0.07f * (float)sr);  /* 70ms por nota */
        vs->gap_len      = (uint32_t)(0.025f * (float)sr); /* 25ms pausa */
        vs->total_samples = vs->repeat_max * (vs->repeat_len + vs->gap_len);
        break;

    /* ── CHIRP_CURIOUS: pio de pergunta ascendente ──────────────────────── */
    case VOICE_CHIRP_CURIOUS:
        vs->f0           = 380.0f;
        vs->f1           = 760.0f;    /* sobe uma oitava — interrogativo */
        vs->vibrato_rate  = 7.0f;     /* vibrato no topo (hesitação curiosa) */
        vs->vibrato_depth = 18.0f;    /* ±18 Hz */
        vs->tremolo_rate  = 0.0f;
        vs->repeat_max   = 0;
        break;

    /* ── GRUNT_IDLE: bocejo/despertar sonolento ─────────────────────────── */
    case VOICE_GRUNT_IDLE:
        vs->f0           = 320.0f;   /* começa meio-alto */
        vs->f1           = 160.0f;   /* cai lentamente */
        vs->vibrato_rate  = 0.0f;
        vs->vibrato_depth = 0.0f;
        vs->tremolo_rate  = 4.0f;    /* leve ondulação de bocejo */
        vs->repeat_max   = 0;
        break;

    /* ── WHINE_PROTEST: reclamação/saudade descendente ─────────────────── */
    case VOICE_WHINE_PROTEST:
        vs->f0           = 320.0f;
        vs->f1           = 190.0f;   /* desce — tristeza / reclamação */
        vs->vibrato_rate  = 6.5f;
        vs->vibrato_depth = 25.0f * vs->intensity; /* mais intenso = mais angústia */
        vs->tremolo_rate  = 7.0f;    /* choro pulsante */
        vs->repeat_max   = 0;
        break;

    /* ── GROWL_ANNOYED: growl grave de raiva baixa ──────────────────────── */
    case VOICE_GROWL_ANNOYED:
        vs->f0           = 160.0f + 60.0f * vs->intensity; /* mais bravo = mais grave */
        vs->f1           = 0.0f;     /* sem sweep */
        vs->f2           = vs->f0 * 2.5f; /* harmônico irregular */
        vs->vibrato_rate  = 0.0f;
        vs->vibrato_depth = 0.0f;
        vs->tremolo_rate  = 11.0f;   /* rasgar de garganta */
        vs->repeat_max   = 0;
        break;

    /* ── HISS_ANGRY: burst de ruído fricativo (cuspida/sísio) ──────────── */
    case VOICE_HISS_ANGRY:
        /* puramente ruído — não usa osciladores, apenas noise_seed */
        vs->f0           = 0.0f;
        vs->f1           = 0.0f;
        vs->repeat_max   = 0;
        break;

    /* ── SQUEAK_STARTLED: susto agudo instantâneo descendente ──────────── */
    case VOICE_SQUEAK_STARTLED:
        vs->f0           = 1300.0f;  /* começa muito agudo */
        vs->f1           = 550.0f;   /* cai rapidamente */
        vs->vibrato_rate  = 0.0f;
        vs->vibrato_depth = 0.0f;
        vs->tremolo_rate  = 0.0f;
        vs->repeat_max   = 0;
        break;

    /* ── CRY_DISTRESS: choro de angústia com vibrato intenso ───────────── */
    case VOICE_CRY_DISTRESS:
        vs->f0           = 380.0f;
        vs->f1           = 380.0f;   /* frequência quase estável */
        vs->vibrato_rate  = 7.5f;
        vs->vibrato_depth = 40.0f * vs->intensity; /* choro intenso */
        vs->tremolo_rate  = 7.5f;    /* amplitude pulsante */
        vs->repeat_max   = 0;
        break;

    default:
        vs->total_samples = 0;
        vs->current_type  = VOICE_IDLE;
        break;
    }

    /* Modulação ontogenética de pitch (maturação vocal de filhote a veterano) */
    if (vs->pitch_scale > 0.1f && vs->current_type != VOICE_IDLE) {
        vs->f0 *= vs->pitch_scale;
        vs->f1 *= vs->pitch_scale;
        vs->f2 *= vs->pitch_scale;
    }
}

/* ---------------------------------------------------------------------------
 * voice_synth_trigger — compatibilidade com código anterior
 * --------------------------------------------------------------------------- */
void voice_synth_trigger(VoiceSynth *vs, VoiceType type, float duration_sec) {
    voice_synth_trigger_ex(vs, type, duration_sec, 0.6f);
}

/* ---------------------------------------------------------------------------
 * voice_synth_render — geração de amostras PCM
 * --------------------------------------------------------------------------- */
size_t voice_synth_render(VoiceSynth *vs, int16_t *out_buffer, size_t max_samples) {
    if (!vs || !out_buffer || vs->current_type == VOICE_IDLE) return 0;

    size_t count = 0;
    float  dt    = 1.0f / (float)vs->sample_rate;

    while (count < max_samples && vs->sample_index < vs->total_samples) {

        /* t = posição normalizada 0.0→1.0 dentro do total */
        float t = (float)vs->sample_index / (float)vs->total_samples;

        float sample = 0.0f;
        float gain   = 11000.0f;

        /* ── Vibrato: desvio de fase acumulado ─────────────────────────── */
        float vib_offset = 0.0f;
        if (vs->vibrato_rate > 0.0f) {
            vib_offset = vs->vibrato_depth * sinf(vs->vibrato_phase);
        }

        /* ── Tremolo: fator de amplitude 0.5→1.0 ──────────────────────── */
        float trem = 1.0f;
        if (vs->tremolo_rate > 0.0f) {
            trem = 0.6f + 0.4f * sinf(vs->tremolo_phase);
        }

        switch (vs->current_type) {

        /* ────────────────────────────────────────────────────────────────
         * PURR_CONTENT: ronron de carinho moderado
         *   - Carrier: sine f0=68Hz
         *   - AM: LFO triangular ~20Hz (inhale/exhale rítmico)
         *   - Harmônico: 1 oitava acima em 20% de amplitude
         *   - Leve ruído orgânico +4%
         * ──────────────────────────────────────────────────────────────── */
        case VOICE_PURR_CONTENT: {
            float cur_f   = vs->f0 + vib_offset;
            float carrier = sinf(vs->phase);
            float harmonic= sinf(vs->phase * 2.0f) * 0.20f;
            float noise   = noise_next(&vs->noise_seed) * 0.04f;
            /* AM com forma de onda triangular aproximada (mais orgânico que sine) */
            float lfo_sin = sinf(vs->mod_phase);
            float lfo     = 0.55f + 0.45f * (lfo_sin > 0 ? lfo_sin : -lfo_sin * 0.6f);
            sample = (carrier + harmonic + noise) * lfo;
            gain   = 10000.0f + 2000.0f * vs->intensity;
            vs->phase     += TWO_PI * cur_f * dt;
            vs->mod_phase += TWO_PI * vs->f1 * dt;
            /* Envelope: começa suave, sustentado */
            float env = envelope(t, 0.08f, 0.12f);
            sample *= env;
            break;
        }

        /* ────────────────────────────────────────────────────────────────
         * PURR_DEEP: ronron profundo de êxtase
         *   - f0=55Hz + 2º harmônico 30% + 3º 15%
         *   - AM mais rápida e profunda
         *   - Vibrato leve de êxtase
         * ──────────────────────────────────────────────────────────────── */
        case VOICE_PURR_DEEP: {
            float cur_f    = vs->f0 + vib_offset;
            float carrier  = sinf(vs->phase);
            float h2       = sinf(vs->phase * 2.0f) * 0.30f;
            float h3       = sinf(vs->phase * 3.0f) * 0.15f;
            float noise    = noise_next(&vs->noise_seed) * 0.05f;
            float lfo_deep = 0.45f + 0.55f * fabsf(sinf(vs->mod_phase));
            sample = (carrier + h2 + h3 + noise) * lfo_deep;
            gain   = 11000.0f + 3000.0f * vs->intensity;
            vs->phase     += TWO_PI * cur_f * dt;
            vs->mod_phase += TWO_PI * vs->f1 * dt;
            float env = envelope(t, 0.06f, 0.10f);
            sample *= env;
            break;
        }

        /* ────────────────────────────────────────────────────────────────
         * TRILL: trilo de alegria — 3 chirps rápidos com gap
         *   - Usa repeat_len / gap_len para gerir as repetições in-place
         *   - Dentro do chirp: varredura f0→f1 rápida
         *   - No gap: silêncio (sample=0)
         * ──────────────────────────────────────────────────────────────── */
        case VOICE_TRILL: {
            uint32_t period = vs->repeat_len + vs->gap_len;
            uint32_t pos_in_period = vs->sample_index % period;
            if (pos_in_period < vs->repeat_len) {
                float t_chirp  = (float)pos_in_period / (float)vs->repeat_len;
                float cur_f    = vs->f0 + (vs->f1 - vs->f0) * t_chirp;
                sample = sinf(vs->phase);
                /* envelope do chirp individual */
                sample *= envelope(t_chirp, 0.1f, 0.15f);
                gain = 11000.0f;
                vs->phase += TWO_PI * cur_f * dt;
            } else {
                /* gap silencioso — reseta fase para próximo chirp limpo */
                sample     = 0.0f;
                vs->phase  = 0.0f;
            }
            break;
        }

        /* ────────────────────────────────────────────────────────────────
         * CHIRP_HAPPY: 2 notas curtas (nota1 → pausa → nota2 mais alta)
         * ──────────────────────────────────────────────────────────────── */
        case VOICE_CHIRP_HAPPY: {
            uint32_t period       = vs->repeat_len + vs->gap_len;
            uint32_t pos_in_period= vs->sample_index % period;
            uint8_t  note_idx     = (uint8_t)(vs->sample_index / period);
            float    note_f       = (note_idx == 0) ? vs->f0 : vs->f1;

            if (pos_in_period < vs->repeat_len) {
                float t_note = (float)pos_in_period / (float)vs->repeat_len;
                sample = sinf(vs->phase) * envelope(t_note, 0.08f, 0.15f);
                gain   = 11500.0f;
                vs->phase += TWO_PI * note_f * dt;
            } else {
                sample    = 0.0f;
                vs->phase = 0.0f;
            }
            break;
        }

        /* ────────────────────────────────────────────────────────────────
         * CHIRP_CURIOUS: pio interrogativo ascendente com vibrato no topo
         *   - f0→f1 ascendente
         *   - Vibrato entra nos últimos 30% da duração
         * ──────────────────────────────────────────────────────────────── */
        case VOICE_CHIRP_CURIOUS: {
            float sweep_f  = vs->f0 + (vs->f1 - vs->f0) * t;
            /* vibrato cresce ao chegar no topo */
            float vib_mix  = (t > 0.65f) ? (t - 0.65f) / 0.35f : 0.0f;
            float cur_f    = sweep_f + vib_offset * vib_mix;
            sample = sinf(vs->phase);
            gain   = 10500.0f;
            vs->phase += TWO_PI * cur_f * dt;
            float env = envelope(t, 0.06f, 0.18f);
            sample *= env;
            break;
        }

        /* ────────────────────────────────────────────────────────────────
         * GRUNT_IDLE: bocejo/despertar — descida lenta com tremolo suave
         *   - Amplitude cresce até 40% depois cai com o bocejo
         * ──────────────────────────────────────────────────────────────── */
        case VOICE_GRUNT_IDLE: {
            float cur_f  = vs->f0 + (vs->f1 - vs->f0) * t;
            /* envelope de bocejo: sobe até 0.35 depois cai */
            float env    = (t < 0.35f) ? (t / 0.35f) : (1.0f - t) / 0.65f;
            float noise  = noise_next(&vs->noise_seed) * 0.08f; /* rouquidão */
            sample = (sinf(vs->phase) + noise) * env * trem;
            gain   = 9000.0f;
            vs->phase += TWO_PI * cur_f * dt;
            break;
        }

        /* ────────────────────────────────────────────────────────────────
         * WHINE_PROTEST: queda expressiva com vibrato de angústia
         *   - f0→f1 descendente + vibrato + tremolo pulsante
         * ──────────────────────────────────────────────────────────────── */
        case VOICE_WHINE_PROTEST: {
            float cur_f   = vs->f0 + (vs->f1 - vs->f0) * t + vib_offset;
            /* vibrato cresce com o tempo — saudade aumenta */
            float vib_mix = t;
            cur_f += vib_offset * vib_mix;
            sample = sinf(vs->phase) * trem;
            gain   = 10000.0f + 1500.0f * vs->intensity;
            vs->phase += TWO_PI * cur_f * dt;
            float env = envelope(t, 0.05f, 0.20f);
            sample *= env;
            break;
        }

        /* ────────────────────────────────────────────────────────────────
         * GROWL_ANNOYED: growl grave de raiva baixa
         *   - f0 estável grave + harmônicos fortes + tremolo rápido (rasgar)
         *   - Adiciona ruído irregular para textura gutural
         * ──────────────────────────────────────────────────────────────── */
        case VOICE_GROWL_ANNOYED: {
            float cur_f   = vs->f0 + vib_offset;
            float h1      = sinf(vs->phase);
            float h2      = sinf(vs->phase * 2.0f) * 0.50f;
            float h3      = sinf(vs->phase * 3.0f) * 0.30f;
            float h4      = sinf(vs->phase * 4.5f) * 0.15f; /* inarmônico = growl */
            float noise   = noise_next(&vs->noise_seed) * 0.10f;
            sample = (h1 + h2 + h3 + h4 + noise) * trem;
            gain   = 9500.0f + 2000.0f * vs->intensity;
            vs->phase     += TWO_PI * cur_f * dt;
            float env = envelope(t, 0.03f, 0.10f);
            sample *= env;
            break;
        }

        /* ────────────────────────────────────────────────────────────────
         * HISS_ANGRY: burst de ruído fricativo puro (sísio/cuspida)
         *   - Apenas ruído branco com ataque instantâneo e decay rápido
         * ──────────────────────────────────────────────────────────────── */
        case VOICE_HISS_ANGRY: {
            float noise  = noise_next(&vs->noise_seed);
            /* ataque zero — decay exponencial rápido */
            float env    = (1.0f - t);
            env = env * env; /* quadrático = decay mais rápido */
            sample = noise * env;
            gain   = 20000.0f * vs->intensity;
            break;
        }

        /* ────────────────────────────────────────────────────────────────
         * SQUEAK_STARTLED: susto agudo — transiente descendente
         *   - 1300→550 Hz em tempo muito curto
         *   - Ataque zero (instantâneo), decay rápido
         * ──────────────────────────────────────────────────────────────── */
        case VOICE_SQUEAK_STARTLED: {
            float cur_f  = vs->f0 + (vs->f1 - vs->f0) * t;
            /* envelope: instantâneo no ataque, decay quadrático */
            float env    = (1.0f - t);
            env = env * env;
            sample = sinf(vs->phase) * env;
            gain   = 14000.0f;
            vs->phase += TWO_PI * cur_f * dt;
            break;
        }

        /* ────────────────────────────────────────────────────────────────
         * CRY_DISTRESS: choro de angústia — vibrato intenso pulsante
         * ──────────────────────────────────────────────────────────────── */
        case VOICE_CRY_DISTRESS: {
            float cur_f   = vs->f0 + vib_offset;
            sample = sinf(vs->phase) * trem;
            /* envelope de choro: fades suaves */
            float env = envelope(t, 0.08f, 0.15f);
            /* pulsação adicional (soluço) */
            float sob = 0.75f + 0.25f * sinf(vs->phase2);
            sample *= env * sob;
            gain   = 11000.0f + 2500.0f * vs->intensity;
            vs->phase  += TWO_PI * cur_f * dt;
            vs->phase2 += TWO_PI * 3.5f * dt; /* soluço ~3.5 Hz */
            break;
        }

        default:
            sample = 0.0f;
            break;
        }

        /* Avança fases de vibrato e tremolo para todos os tipos */
        if (vs->vibrato_rate > 0.0f)
            vs->vibrato_phase += TWO_PI * vs->vibrato_rate * dt;
        if (vs->tremolo_rate > 0.0f)
            vs->tremolo_phase += TWO_PI * vs->tremolo_rate * dt;

        /* Wrap fases para evitar overflow de float */
        if (vs->phase        > TWO_PI * 1000.0f) vs->phase        -= TWO_PI * 1000.0f;
        if (vs->mod_phase    > TWO_PI * 1000.0f) vs->mod_phase    -= TWO_PI * 1000.0f;
        if (vs->vibrato_phase> TWO_PI * 1000.0f) vs->vibrato_phase-= TWO_PI * 1000.0f;
        if (vs->tremolo_phase> TWO_PI * 1000.0f) vs->tremolo_phase-= TWO_PI * 1000.0f;

        /* Escala e clamp para int16 */
        out_buffer[count++] = f_to_i16(sample * gain);
        vs->sample_index++;
    }

    if (vs->sample_index >= vs->total_samples) {
        vs->current_type = VOICE_IDLE;
    }

    return count;
}
