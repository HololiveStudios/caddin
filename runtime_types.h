#ifndef PESTINHA_RUNTIME_TYPES_H
#define PESTINHA_RUNTIME_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#define PESTINHA_CHANNELS 64
#define PESTINHA_INTERNEURONS 32
#define PESTINHA_MOTORS 24
#ifndef PESTINHA_NEURON_SCALE
#define PESTINHA_NEURON_SCALE 512
#endif
#define PESTINHA_SENSORY_PER_POOL (8 * PESTINHA_NEURON_SCALE)
#define PESTINHA_INTER_PER_POOL   (36 * PESTINHA_NEURON_SCALE)
#define PESTINHA_MOTOR_PER_POOL   (16 * PESTINHA_NEURON_SCALE)
#define PESTINHA_NEURONS \
    ((PESTINHA_CHANNELS * PESTINHA_SENSORY_PER_POOL) + \
     (PESTINHA_INTERNEURONS * PESTINHA_INTER_PER_POOL) + \
     (PESTINHA_MOTORS * PESTINHA_MOTOR_PER_POOL))

/* Índices únicos dos canais usados pelo codificador e pelos sensores do SO. */
typedef enum {
    CH_TYPING = 0, CH_KEY_FAST, CH_KEY_SLOW, CH_IDLE, CH_PAUSE,
    CH_CMD_ENTER, CH_BACKSPACE, CH_DELETE, CH_ARROW, CH_TAB,
    CH_SPACE, CH_SHIFT, CH_ALPHANUM, CH_NUMERIC, CH_SYMBOL, CH_PUNCTUATION,
    CH_CARESS, CH_POKE, CH_FEED, CH_TALK, CH_GREET, CH_IGNORE,
    CH_REWARD, CH_PUNISH, CH_HELP_REQ, CH_CLEAN, CH_PLAY,
    CH_SLEEP_REQ, CH_CALL, CH_WHISTLE, CH_MUSIC, CH_NOISE,
    CH_TIME_DAY, CH_TIME_NIGHT, CH_UPTIME, CH_CPU_LOAD,
    CH_IDLE_LONG, CH_FOCUS_ACTIVE, CH_SESSION_LEN, CH_TICK_PULSE,
    CH_TEMP_WARM, CH_TEMP_COLD, CH_USER_PRESENT, CH_USER_AWAY,
    CH_QUICK_CLICK, CH_SLOW_CLICK, CH_BURST_INPUT, CH_RHYTHM_REGULAR,
    CH_CIRCADIAN_PEAK, CH_CIRCADIAN_LOW, CH_FATIGUE_PULSE,
    CH_HUNGER_PULSE, CH_HEARTBEAT_FAST, CH_HEARTBEAT_SLOW,
    CH_BREATHING, CH_ADRENALINE, CH_DOPAMINE, CH_SEROTONIN,
    CH_CORTISOL, CH_OXYTOCIN, CH_ENDORPHIN, CH_MEMORY_TRACE,
    CH_STDP_SPIKE, CH_REWARD_WAIT
} SensorChannel;

_Static_assert(CH_REWARD_WAIT + 1 == PESTINHA_CHANNELS,
               "A enumeração de canais precisa corresponder a PESTINHA_CHANNELS.");

/* Perfil Carmin-alvo: escala 512 sobre os 120 grupos existentes.
   Sensoriais: 64 pools * 4096 = 262144
   Interneurônios: 32 pools * 18432 = 589824
   Motores: 24 pools * 8192 = 196608
   Total: 1048576 neurônios. Os IDs de grupos permanecem estáveis.
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
