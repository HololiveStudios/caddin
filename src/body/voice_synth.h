#ifndef PET_BODY_VOICE_SYNTH_H
#define PET_BODY_VOICE_SYNTH_H

#include <stdint.h>
#include <stddef.h>

/* ---------------------------------------------------------------------------
 * Tipos de voz emocional procedural
 * Cada tipo mapeia para uma emoção específica com síntese biologicamente
 * inspirada em vocalizações felinas/mamíferas.
 * --------------------------------------------------------------------------- */
typedef enum {
    VOICE_IDLE = 0,

    /* ── Positivo / Afeto ─────────────────────────────────────────── */
    VOICE_PURR_CONTENT,     /* carinho moderado: ronron grave com AM suave       */
    VOICE_PURR_DEEP,        /* êxtase de carinho prolongado: ronron profundo      */
    VOICE_TRILL,            /* alegria/brincadeira: trilo rápido 3x ascendente   */
    VOICE_CHIRP_HAPPY,      /* felicidade: 2 notas curtas e redondas              */

    /* ── Curioso / Neutro ─────────────────────────────────────────── */
    VOICE_CHIRP_CURIOUS,    /* pio de pergunta: ascendente com vibrato no topo   */
    VOICE_GRUNT_IDLE,       /* bocejo/despertar: descida lenta e sonolenta        */

    /* ── Negativo / Desconforto ───────────────────────────────────── */
    VOICE_WHINE_PROTEST,    /* reclamação suave: descida com tremolo de saudade  */
    VOICE_GROWL_ANNOYED,    /* raiva baixa sustentada: growl grave com harmônicos */
    VOICE_HISS_ANGRY,       /* raiva intensa: burst de ruído fricativo             */
    VOICE_SQUEAK_STARTLED,  /* susto agudo: transiente descendente instantâneo   */
    VOICE_CRY_DISTRESS,     /* angústia/dor: vibrato intenso pulsante             */

    VOICE_TYPE_COUNT
} VoiceType;

/* ---------------------------------------------------------------------------
 * Estado completo do sintetizador (zero alocação dinâmica)
 * --------------------------------------------------------------------------- */
typedef struct {
    VoiceType current_type;
    uint32_t  sample_rate;
    uint32_t  sample_index;
    uint32_t  total_samples;

    /* Osciladores principais */
    float     f0;           /* frequência fundamental (Hz)                       */
    float     f1;           /* frequência alvo / LFO rate                        */
    float     f2;           /* 3ª frequência (formante 2 / harmônico extra)      */
    float     phase;        /* fase do oscilador principal                       */
    float     mod_phase;    /* fase do LFO / oscilador secundário                */
    float     phase2;       /* fase do oscilador terciário                       */

    /* Vibrato (trêmulo de pitch) */
    float     vibrato_phase;
    float     vibrato_rate; /* Hz, tipicamente 5-8 Hz                           */
    float     vibrato_depth;/* profundidade em Hz                                */

    /* Tremolo (modulação de amplitude) */
    float     tremolo_phase;
    float     tremolo_rate; /* Hz, 12-25 para ronron, 6-10 para whine            */

    /* Pseudo-ruído para texturas orgânicas (xorshift32 sem malloc) */
    uint32_t  noise_seed;

    /* Controle de repetição (trilos) */
    uint8_t   repeat_count; /* repetições já feitas                              */
    uint8_t   repeat_max;   /* total de repetições planejadas                    */
    uint32_t  repeat_len;   /* duração de cada repetição em amostras             */
    uint32_t  gap_len;      /* duração do gap entre repetições em amostras       */

    /* Intensidade emocional 0..1 (modula volume, vibrato, profundidade) */
    float     intensity;

    /* Fase do envelope de respiração (para purr inhale/exhale) */
    float     breath_phase;
} VoiceSynth;

/* ---------------------------------------------------------------------------
 * API pública
 * --------------------------------------------------------------------------- */
void   voice_synth_init(VoiceSynth *vs, uint32_t sample_rate);

/* Trigger simples (compatibilidade) */
void   voice_synth_trigger(VoiceSynth *vs, VoiceType type, float duration_sec);

/* Trigger rico com intensidade emocional (0.0 = mínimo, 1.0 = máximo) */
void   voice_synth_trigger_ex(VoiceSynth *vs, VoiceType type,
                               float duration_sec, float intensity);

/* Gera amostras PCM 16-bit mono. Retorna qtd de amostras geradas (0 = silêncio). */
size_t voice_synth_render(VoiceSynth *vs, int16_t *out_buffer, size_t max_samples);

#endif /* PET_BODY_VOICE_SYNTH_H */
