#ifndef PESTINHA_RUNTIME_TYPES_H
#define PESTINHA_RUNTIME_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#define PESTINHA_CHANNELS 64
#define PESTINHA_INTERNEURONS 32
#define PESTINHA_MOTORS 24
#define PESTINHA_NEURONS 2048

/* Sensoriais: 64 canais * 4 neurônios = 256
   Interneurônios: 32 pools * 18 neurônios = 576
   Motores: 24 pools * 8 neurônios = 192
   Total: 256 + 576 + 192 = 1024 neurônios
*/

typedef struct {
    double energy;      /* 0.0 a 1.0 */
    double stress;      /* 0.0 a 1.0 (0.0 = calmo, 1.0 = muito estressado) */
    double curiosity;   /* 0.0 a 1.0 */
    double satiety;     /* 0.0 a 1.0 */
    double affection;   /* 0.0 a 1.0 (vínculo com o usuário) */
} Drives;

typedef struct {
    double valence;     /* -1.0 (negativo/triste) a +1.0 (positivo/feliz) */
    double arousal;     /* 0.0 (calmo/dormindo) a 1.0 (agitado/empolgado) */
    double dominance;   /* -1.0 a +1.0 */
    double mood;        /* estado emocional integrado */
    int current_emotion;
} Emotion;


typedef struct {
    bool asleep;
    bool sitting;
    bool wandering;
    bool curious;
    bool startled;
    bool playful;
    bool eating;
    double state_timer;
} Behavior;

typedef struct {
    double typing_rate;
    double idle_time;
    double last_key_time;
    char last_char;
    int key_events_count;
} SensorReading;

typedef struct {
    double reward_trace;
    double punishment_trace;
    double caress_count;
    double poke_count;
} ConditioningState;

typedef struct {
    double last_caress_t;
    double last_poke_t;
    double last_feed_t;
    int positive_interactions;
    int negative_interactions;
} EpisodeMemory;

typedef struct {
    int term_width;
    int term_height;
    bool ansi_supported;
    bool running;
} TerminalState;

#endif /* PESTINHA_RUNTIME_TYPES_H */
