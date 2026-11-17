#ifndef PESTINHA_NETWORK_DATA_H
#define PESTINHA_NETWORK_DATA_H

#include "runtime_types.h"

typedef struct {
    const char *name;
    int n;          /* número de neurônios no pool */
    double rate;    /* taxa base de disparo */
    double d;       /* depressão */
    double tau_rec; /* tempo de recuperação */
} SensoryPoolDef;

typedef struct {
    const char *name;
    int n;          /* número de neurônios no pool */
    const char *kind; /* "exc" ou "inh" */
    double tau;     /* constante de tempo de membrana (ms) */
    double b;       /* adaptação */
    double bt;      /* tempo de recuperação da adaptação */
} InterPoolDef;

typedef struct {
    const char *name;
    int n;          /* número de neurônios no pool */
    double tau;     /* constante de tempo */
    double b;       /* adaptação */
    double bt;      /* tempo de recuperação */
} MotorPoolDef;

typedef struct {
    const char *pre;
    const char *post;
    double weight;
    double density;
} ConnDef;

/* Definition of 64 Sensory Channels */
static const SensoryPoolDef SENS_POOLS[PESTINHA_CHANNELS] = {
    {"TYPING", 8, 30.0, 0.15, 20.0},
    {"KEY_FAST", 8, 40.0, 0.20, 15.0},
    {"KEY_SLOW", 8, 15.0, 0.10, 30.0},
    {"IDLE", 8, 10.0, 0.05, 50.0},
    {"PAUSE", 8, 12.0, 0.05, 40.0},
    {"CMD_ENTER", 8, 50.0, 0.25, 10.0},
    {"BACKSPACE", 8, 35.0, 0.15, 20.0},
    {"DELETE", 8, 35.0, 0.15, 20.0},
    {"ARROW", 8, 25.0, 0.10, 25.0},
    {"TAB", 8, 20.0, 0.10, 25.0},
    {"SPACE", 8, 30.0, 0.12, 20.0},
    {"SHIFT", 8, 20.0, 0.08, 30.0},
    {"ALPHANUM", 8, 25.0, 0.10, 20.0},
    {"NUMERIC", 8, 25.0, 0.10, 20.0},
    {"SYMBOL", 8, 30.0, 0.12, 20.0},
    {"PUNCTUATION", 8, 20.0, 0.10, 25.0},

    {"CARESS", 8, 60.0, 0.30, 10.0},
    {"POKE", 8, 70.0, 0.40, 10.0},
    {"FEED", 8, 50.0, 0.25, 15.0},
    {"TALK", 8, 45.0, 0.20, 15.0},
    {"GREET", 8, 40.0, 0.18, 20.0},
    {"IGNORE", 8, 15.0, 0.05, 40.0},
    {"REWARD", 8, 65.0, 0.30, 10.0},
    {"PUNISH", 8, 65.0, 0.35, 10.0},
    {"HELP_REQ", 8, 30.0, 0.15, 25.0},
    {"CLEAN", 8, 35.0, 0.15, 20.0},
    {"PLAY", 8, 50.0, 0.22, 15.0},
    {"SLEEP_REQ", 8, 25.0, 0.10, 30.0},
    {"CALL", 8, 45.0, 0.20, 15.0},
    {"WHISTLE", 8, 40.0, 0.18, 20.0},
    {"MUSIC", 8, 35.0, 0.12, 25.0},
    {"NOISE", 8, 55.0, 0.25, 15.0},

    {"TIME_DAY", 8, 10.0, 0.02, 100.0},
    {"TIME_NIGHT", 8, 10.0, 0.02, 100.0},
    {"UPTIME", 8, 15.0, 0.05, 50.0},
    {"CPU_LOAD", 8, 20.0, 0.08, 30.0},
    {"IDLE_LONG", 8, 8.0, 0.02, 120.0},
    {"FOCUS_ACTIVE", 8, 25.0, 0.10, 25.0},
    {"SESSION_LEN", 8, 12.0, 0.04, 80.0},
    {"TICK_PULSE", 8, 20.0, 0.08, 30.0},
    {"TEMP_WARM", 8, 15.0, 0.05, 50.0},
    {"TEMP_COLD", 8, 15.0, 0.05, 50.0},
    {"USER_PRESENT", 8, 30.0, 0.10, 20.0},
    {"USER_AWAY", 8, 10.0, 0.03, 80.0},
    {"QUICK_CLICK", 8, 35.0, 0.15, 20.0},
    {"SLOW_CLICK", 8, 15.0, 0.05, 40.0},
    {"BURST_INPUT", 8, 50.0, 0.22, 15.0},
    {"RHYTHM_REGULAR", 8, 25.0, 0.10, 25.0},

    {"CIRCADIAN_PEAK", 8, 12.0, 0.04, 80.0},
    {"CIRCADIAN_LOW", 8, 12.0, 0.04, 80.0},
    {"FATIGUE_PULSE", 8, 18.0, 0.06, 40.0},
    {"HUNGER_PULSE", 8, 18.0, 0.06, 40.0},
    {"HEARTBEAT_FAST", 8, 30.0, 0.12, 20.0},
    {"HEARTBEAT_SLOW", 8, 15.0, 0.05, 50.0},
    {"BREATHING", 8, 20.0, 0.08, 30.0},
    {"ADRENALINE", 8, 45.0, 0.20, 15.0},
    {"DOPAMINE", 8, 50.0, 0.22, 15.0},
    {"SEROTONIN", 8, 25.0, 0.10, 25.0},
    {"CORTISOL", 8, 40.0, 0.18, 18.0},
    {"OXYTOCIN", 8, 35.0, 0.15, 20.0},
    {"ENDORPHIN", 8, 40.0, 0.18, 18.0},
    {"MEMORY_TRACE", 8, 20.0, 0.08, 30.0},
    {"STDP_SPIKE", 8, 30.0, 0.12, 20.0},
    {"REWARD_WAIT", 8, 22.0, 0.09, 25.0}
};

/* Definition of 32 Interneuron Pools (576 neurons total) */
static const InterPoolDef INTER_POOLS[PESTINHA_INTERNEURONS] = {
    {"ALERT", 36, "exc", 20.0, 0.05, 100.0},
    {"CURIOUS", 36, "exc", 22.0, 0.04, 120.0},
    {"CALM", 36, "exc", 25.0, 0.03, 150.0},
    {"PLAYFUL", 36, "exc", 18.0, 0.06, 90.0},
    {"FRUSTRATION", 36, "exc", 16.0, 0.07, 80.0},
    {"GLOOM", 36, "exc", 30.0, 0.02, 200.0},
    {"SLEEPY", 36, "exc", 35.0, 0.02, 220.0},
    {"HUNGRY", 36, "exc", 24.0, 0.04, 130.0},
    {"SATISFIED", 36, "exc", 26.0, 0.03, 140.0},
    {"BORED", 36, "exc", 28.0, 0.03, 160.0},
    {"SURPRISED", 36, "exc", 14.0, 0.08, 70.0},
    {"FEARFUL", 36, "exc", 15.0, 0.08, 75.0},
    {"AFFECTIONATE", 36, "exc", 22.0, 0.04, 110.0},
    {"LONELY", 36, "exc", 32.0, 0.02, 190.0},
    {"HYPERACTIVE", 36, "exc", 12.0, 0.10, 60.0},
    {"STUBBORN", 36, "exc", 20.0, 0.05, 120.0},
    {"CREATIVE", 36, "exc", 21.0, 0.05, 115.0},
    {"MEMORY_HOLD", 36, "exc", 40.0, 0.01, 300.0},
    {"REWARD_EXPECT", 36, "exc", 19.0, 0.06, 100.0},
    {"THREAT_DETECT", 36, "inh", 15.0, 0.08, 70.0},
    {"RHYTHM_GEN", 36, "exc", 18.0, 0.05, 90.0},
    {"FOCUS_POOL", 36, "exc", 23.0, 0.04, 130.0},
    {"COGNITIVE_LOW", 36, "inh", 30.0, 0.02, 180.0},
    {"COGNITIVE_HIGH", 36, "exc", 17.0, 0.07, 85.0},
    {"SOCIAL_SEEK", 36, "exc", 25.0, 0.04, 140.0},
    {"PAIN_REACTION", 36, "inh", 14.0, 0.09, 65.0},
    {"COMFORT_POOL", 36, "exc", 27.0, 0.03, 150.0},
    {"ENERGY_CONSERVE", 36, "exc", 35.0, 0.02, 210.0},
    {"HABITUATION_POOL", 36, "inh", 28.0, 0.03, 160.0},
    {"PLASTIC_NODE1", 36, "exc", 20.0, 0.05, 100.0},
    {"PLASTIC_NODE2", 36, "exc", 20.0, 0.05, 100.0},
    {"EXECUTIVE_CTRL", 36, "exc", 18.0, 0.06, 90.0}
};

/* Definition of 24 Motor Pools (192 neurons total) */
static const MotorPoolDef MOTOR_POOLS[PESTINHA_MOTORS] = {
    {"FACE_HAPPY", 16, 20.0, 0.05, 100.0},
    {"FACE_SAD", 16, 25.0, 0.04, 120.0},
    {"FACE_SLEEP", 16, 30.0, 0.02, 180.0},
    {"FACE_SURPRISED", 16, 15.0, 0.08, 70.0},
    {"FACE_ANGRY", 16, 16.0, 0.07, 80.0},
    {"FACE_CURIOUS", 16, 22.0, 0.05, 110.0},
    {"FACE_PLAYFUL", 16, 18.0, 0.06, 90.0},
    {"FACE_NEUTRAL", 16, 25.0, 0.03, 140.0},
    {"FACE_WINK", 16, 17.0, 0.07, 85.0},
    {"CHIRP_SOUND", 16, 14.0, 0.08, 65.0},
    {"BEEP_PULSE", 16, 15.0, 0.08, 70.0},
    {"PURR_EFFECT", 16, 24.0, 0.04, 130.0},
    {"WANDER_LEFT", 16, 20.0, 0.05, 100.0},
    {"WANDER_RIGHT", 16, 20.0, 0.05, 100.0},
    {"JUMP_ACTION", 16, 14.0, 0.09, 60.0},
    {"DANCE_ACTION", 16, 16.0, 0.07, 80.0},
    {"SLEEP_ACTION", 16, 35.0, 0.02, 200.0},
    {"EAT_ACTION", 16, 22.0, 0.05, 110.0},
    {"CUDDLE_ACTION", 16, 23.0, 0.04, 120.0},
    {"ALERT_EMIT", 16, 15.0, 0.08, 75.0},
    {"NOD_HEAD", 16, 18.0, 0.06, 90.0},
    {"SHAKE_HEAD", 16, 18.0, 0.06, 90.0},
    {"THINK_EMIT", 16, 21.0, 0.05, 105.0},
    {"TAIL_WAG", 16, 19.0, 0.06, 95.0}
};

/* Connectivity Map — Complete sensory-inter-motor wiring */
static const ConnDef DEFAULT_CONNS[] = {
    /* ═══ Sensores de Digitação/Teclas → Interneurônios ═══ */
    {"TYPING",         "ALERT",          0.45, 0.35},
    {"TYPING",         "FOCUS_POOL",     0.40, 0.30},
    {"KEY_FAST",       "HYPERACTIVE",    0.50, 0.40},
    {"KEY_FAST",       "ALERT",          0.35, 0.30},
    {"KEY_SLOW",       "CALM",           0.30, 0.25},
    {"KEY_SLOW",       "FOCUS_POOL",     0.35, 0.30},
    {"IDLE",           "SLEEPY",         0.40, 0.30},
    {"IDLE",           "BORED",          0.35, 0.30},
    {"PAUSE",          "CALM",           0.30, 0.25},
    {"CMD_ENTER",      "ALERT",          0.50, 0.35},
    {"CMD_ENTER",      "CURIOUS",        0.35, 0.30},
    {"SPACE",          "SURPRISED",      0.45, 0.35},
    {"SPACE",          "HYPERACTIVE",    0.35, 0.30},
    {"ARROW",          "CURIOUS",        0.35, 0.30},
    {"ALPHANUM",       "FOCUS_POOL",     0.30, 0.25},
    {"BURST_INPUT",    "HYPERACTIVE",    0.55, 0.45},
    {"BURST_INPUT",    "FRUSTRATED",     0.40, 0.30},

    /* ═══ Sensores de Interação Direta → Interneurônios ═══ */
    {"CARESS",         "PLAYFUL",        0.60, 0.50},
    {"CARESS",         "AFFECTIONATE",   0.65, 0.55},
    {"CARESS",         "CALM",           0.45, 0.40},
    {"CARESS",         "COMFORT_POOL",   0.50, 0.45},
    {"CARESS",         "SOCIAL_SEEK",    0.40, 0.35},
    {"POKE",           "FRUSTRATION",    0.60, 0.45},
    {"POKE",           "SURPRISED",      0.55, 0.40},
    {"POKE",           "PAIN_REACTION",  0.50, 0.40},
    {"POKE",           "ALERT",          0.40, 0.35},
    {"FEED",           "SATISFIED",      0.70, 0.60},
    {"FEED",           "CALM",           0.50, 0.40},
    {"FEED",           "COMFORT_POOL",   0.45, 0.40},
    {"TALK",           "CURIOUS",        0.45, 0.35},
    {"TALK",           "SOCIAL_SEEK",    0.50, 0.40},
    {"TALK",           "AFFECTIONATE",   0.35, 0.30},
    {"GREET",          "SOCIAL_SEEK",    0.55, 0.45},
    {"GREET",          "CURIOUS",        0.40, 0.35},
    {"IGNORE",         "LONELY",         0.50, 0.40},
    {"IGNORE",         "BORED",          0.40, 0.35},
    {"IGNORE",         "GLOOM",          0.30, 0.25},
    {"PLAY",           "PLAYFUL",        0.65, 0.55},
    {"PLAY",           "HYPERACTIVE",    0.45, 0.40},
    {"PLAY",           "CURIOUS",        0.40, 0.35},
    {"NOISE",          "FEARFUL",        0.55, 0.45},
    {"NOISE",          "ALERT",          0.50, 0.40},
    {"NOISE",          "THREAT_DETECT",  0.60, 0.50},

    /* ═══ Sensores de Recompensa/Punição → Neuromodulação ═══ */
    {"REWARD",         "REWARD_EXPECT",  0.70, 0.60},
    {"REWARD",         "SATISFIED",      0.50, 0.45},
    {"REWARD",         "PLAYFUL",        0.40, 0.35},
    {"PUNISH",         "FRUSTRATION",    0.60, 0.50},
    {"PUNISH",         "FEARFUL",        0.50, 0.45},
    {"PUNISH",         "THREAT_DETECT",  0.55, 0.45},

    /* ═══ Sensores Circadianos e Temporais → Interneurônios ═══ */
    {"TIME_DAY",       "ALERT",          0.25, 0.20},
    {"TIME_DAY",       "CURIOUS",        0.20, 0.18},
    {"TIME_NIGHT",     "SLEEPY",         0.40, 0.35},
    {"TIME_NIGHT",     "CALM",           0.30, 0.25},
    {"IDLE_LONG",      "SLEEPY",         0.50, 0.40},
    {"IDLE_LONG",      "LONELY",         0.40, 0.35},
    {"IDLE_LONG",      "BORED",          0.45, 0.40},
    {"UPTIME",         "COGNITIVE_LOW",  0.30, 0.25},
    {"FOCUS_ACTIVE",   "FOCUS_POOL",     0.50, 0.40},
    {"FOCUS_ACTIVE",   "ALERT",          0.35, 0.30},
    {"USER_PRESENT",   "SOCIAL_SEEK",    0.45, 0.40},
    {"USER_PRESENT",   "CURIOUS",        0.35, 0.30},
    {"USER_AWAY",      "LONELY",         0.45, 0.40},
    {"USER_AWAY",      "BORED",          0.35, 0.30},
    {"QUICK_CLICK",    "ALERT",          0.40, 0.35},
    {"QUICK_CLICK",    "CURIOUS",        0.35, 0.30},
    {"SLOW_CLICK",     "CALM",           0.25, 0.20},
    {"RHYTHM_REGULAR", "RHYTHM_GEN",     0.50, 0.40},
    {"RHYTHM_REGULAR", "FOCUS_POOL",     0.35, 0.30},
    {"CIRCADIAN_PEAK", "HYPERACTIVE",    0.30, 0.25},
    {"CIRCADIAN_PEAK", "COGNITIVE_HIGH", 0.35, 0.30},
    {"CIRCADIAN_LOW",  "SLEEPY",         0.40, 0.35},
    {"CIRCADIAN_LOW",  "ENERGY_CONSERVE",0.35, 0.30},

    /* ═══ Sensores Interoceptivos → Interneurônios ═══ */
    {"FATIGUE_PULSE",  "SLEEPY",         0.55, 0.45},
    {"FATIGUE_PULSE",  "ENERGY_CONSERVE",0.45, 0.40},
    {"HUNGER_PULSE",   "HUNGRY",         0.60, 0.50},
    {"HUNGER_PULSE",   "ALERT",          0.30, 0.25},
    {"HEARTBEAT_FAST", "ALERT",          0.40, 0.35},
    {"HEARTBEAT_FAST", "HYPERACTIVE",    0.35, 0.30},
    {"HEARTBEAT_SLOW", "CALM",           0.35, 0.30},
    {"HEARTBEAT_SLOW", "SLEEPY",         0.25, 0.20},
    {"BREATHING",      "CALM",           0.20, 0.18},
    {"ADRENALINE",     "HYPERACTIVE",    0.55, 0.45},
    {"ADRENALINE",     "ALERT",          0.50, 0.40},
    {"ADRENALINE",     "FEARFUL",        0.40, 0.35},
    {"DOPAMINE",       "PLAYFUL",        0.55, 0.50},
    {"DOPAMINE",       "REWARD_EXPECT",  0.60, 0.50},
    {"DOPAMINE",       "SATISFIED",      0.45, 0.40},
    {"SEROTONIN",      "CALM",           0.45, 0.40},
    {"SEROTONIN",      "SATISFIED",      0.40, 0.35},
    {"SEROTONIN",      "COMFORT_POOL",   0.35, 0.30},
    {"CORTISOL",       "FRUSTRATION",    0.55, 0.45},
    {"CORTISOL",       "FEARFUL",        0.45, 0.40},
    {"CORTISOL",       "THREAT_DETECT",  0.50, 0.40},
    {"OXYTOCIN",       "AFFECTIONATE",   0.60, 0.50},
    {"OXYTOCIN",       "SOCIAL_SEEK",    0.50, 0.45},
    {"OXYTOCIN",       "COMFORT_POOL",   0.45, 0.40},
    {"ENDORPHIN",      "CALM",           0.45, 0.40},
    {"ENDORPHIN",      "SATISFIED",      0.40, 0.35},
    {"ENDORPHIN",      "PLAYFUL",        0.35, 0.30},
    {"MEMORY_TRACE",   "MEMORY_HOLD",    0.50, 0.45},
    {"STDP_SPIKE",     "PLASTIC_NODE1",  0.45, 0.40},
    {"STDP_SPIKE",     "PLASTIC_NODE2",  0.45, 0.40},
    {"REWARD_WAIT",    "REWARD_EXPECT",  0.55, 0.50},
    {"REWARD_WAIT",    "CURIOUS",        0.35, 0.30},

    /* ═══ Interneurônios → Pools Motores (Expressão Facial) ═══ */
    {"PLAYFUL",        "FACE_PLAYFUL",   0.60, 0.50},
    {"PLAYFUL",        "FACE_HAPPY",     0.45, 0.40},
    {"PLAYFUL",        "DANCE_ACTION",   0.55, 0.45},
    {"PLAYFUL",        "TAIL_WAG",       0.50, 0.45},
    {"CALM",           "FACE_HAPPY",     0.50, 0.40},
    {"CALM",           "FACE_NEUTRAL",   0.40, 0.35},
    {"CALM",           "PURR_EFFECT",    0.35, 0.30},
    {"SLEEPY",         "FACE_SLEEP",     0.65, 0.55},
    {"SLEEPY",         "SLEEP_ACTION",   0.70, 0.60},
    {"FRUSTRATED",     "FACE_ANGRY",     0.55, 0.45},
    {"FRUSTRATION",    "FACE_ANGRY",     0.60, 0.50},
    {"FRUSTRATION",    "SHAKE_HEAD",     0.45, 0.40},
    {"GLOOM",          "FACE_SAD",       0.55, 0.45},
    {"BORED",          "FACE_SAD",       0.35, 0.30},
    {"BORED",          "WANDER_LEFT",    0.30, 0.25},
    {"BORED",          "WANDER_RIGHT",   0.30, 0.25},
    {"SURPRISED",      "FACE_SURPRISED", 0.65, 0.55},
    {"SURPRISED",      "ALERT_EMIT",     0.50, 0.45},
    {"FEARFUL",        "FACE_SURPRISED", 0.50, 0.40},
    {"FEARFUL",        "ALERT_EMIT",     0.55, 0.45},
    {"AFFECTIONATE",   "PURR_EFFECT",    0.60, 0.50},
    {"AFFECTIONATE",   "CUDDLE_ACTION",  0.55, 0.50},
    {"AFFECTIONATE",   "FACE_HAPPY",     0.45, 0.40},
    {"AFFECTIONATE",   "TAIL_WAG",       0.45, 0.40},
    {"LONELY",         "FACE_SAD",       0.50, 0.45},
    {"LONELY",         "WANDER_LEFT",    0.35, 0.30},
    {"LONELY",         "WANDER_RIGHT",   0.35, 0.30},
    {"HYPERACTIVE",    "FACE_PLAYFUL",   0.50, 0.45},
    {"HYPERACTIVE",    "JUMP_ACTION",    0.55, 0.50},
    {"HYPERACTIVE",    "DANCE_ACTION",   0.45, 0.40},
    {"HYPERACTIVE",    "TAIL_WAG",       0.40, 0.35},
    {"CURIOUS",        "FACE_CURIOUS",   0.55, 0.45},
    {"CURIOUS",        "WANDER_LEFT",    0.40, 0.35},
    {"CURIOUS",        "WANDER_RIGHT",   0.40, 0.35},
    {"CURIOUS",        "NOD_HEAD",       0.35, 0.30},
    {"SATISFIED",      "FACE_HAPPY",     0.55, 0.50},
    {"SATISFIED",      "PURR_EFFECT",    0.45, 0.40},
    {"SATISFIED",      "TAIL_WAG",       0.40, 0.35},
    {"HUNGRY",         "FACE_SAD",       0.35, 0.30},
    {"HUNGRY",         "EAT_ACTION",     0.55, 0.50},
    {"HUNGRY",         "WANDER_LEFT",    0.25, 0.20},
    {"STUBBORN",       "SHAKE_HEAD",     0.50, 0.45},
    {"CREATIVE",       "FACE_CURIOUS",   0.40, 0.35},
    {"CREATIVE",       "THINK_EMIT",     0.50, 0.45},
    {"CREATIVE",       "DANCE_ACTION",   0.35, 0.30},

    /* ═══ Interneurônios → Motores de Vocalização ═══ */
    {"ALERT",          "CHIRP_SOUND",    0.45, 0.40},
    {"ALERT",          "FACE_SURPRISED", 0.50, 0.40},
    {"ALERT",          "BEEP_PULSE",     0.40, 0.35},
    {"REWARD_EXPECT",  "FACE_HAPPY",     0.45, 0.40},
    {"REWARD_EXPECT",  "TAIL_WAG",       0.50, 0.45},
    {"THREAT_DETECT",  "FACE_ANGRY",     0.50, 0.40},
    {"THREAT_DETECT",  "ALERT_EMIT",     0.55, 0.45},
    {"RHYTHM_GEN",     "DANCE_ACTION",   0.45, 0.40},
    {"RHYTHM_GEN",     "TAIL_WAG",       0.40, 0.35},
    {"FOCUS_POOL",     "FACE_CURIOUS",   0.40, 0.35},
    {"FOCUS_POOL",     "THINK_EMIT",     0.35, 0.30},
    {"COGNITIVE_HIGH", "THINK_EMIT",     0.50, 0.45},
    {"COGNITIVE_HIGH", "NOD_HEAD",       0.35, 0.30},
    {"COGNITIVE_LOW",  "FACE_SLEEP",     0.30, 0.25},
    {"SOCIAL_SEEK",    "FACE_CURIOUS",   0.40, 0.35},
    {"SOCIAL_SEEK",    "WANDER_LEFT",    0.30, 0.25},
    {"SOCIAL_SEEK",    "WANDER_RIGHT",   0.30, 0.25},
    {"SOCIAL_SEEK",    "CHIRP_SOUND",    0.35, 0.30},
    {"PAIN_REACTION",  "FACE_SAD",       0.45, 0.40},
    {"PAIN_REACTION",  "SHAKE_HEAD",     0.40, 0.35},
    {"COMFORT_POOL",   "FACE_HAPPY",     0.40, 0.35},
    {"COMFORT_POOL",   "PURR_EFFECT",    0.45, 0.40},
    {"ENERGY_CONSERVE","SLEEP_ACTION",   0.45, 0.40},
    {"ENERGY_CONSERVE","FACE_SLEEP",     0.40, 0.35},
    {"MEMORY_HOLD",    "THINK_EMIT",     0.35, 0.30},
    {"PLASTIC_NODE1",  "FACE_CURIOUS",   0.30, 0.25},
    {"PLASTIC_NODE2",  "FACE_PLAYFUL",   0.30, 0.25},
    {"EXECUTIVE_CTRL", "NOD_HEAD",       0.40, 0.35},
    {"EXECUTIVE_CTRL", "FACE_NEUTRAL",   0.35, 0.30},

    /* ═══ Circuitos Inibitórios de Balanço ═══ */
    {"THREAT_DETECT",  "PLAYFUL",        -0.40, 0.30},
    {"PAIN_REACTION",  "PLAYFUL",        -0.35, 0.25},
    {"HABITUATION_POOL","ALERT",         -0.30, 0.25},
    {"HABITUATION_POOL","SURPRISED",     -0.35, 0.30},
    {"COGNITIVE_LOW",  "HYPERACTIVE",    -0.30, 0.25},
    {"ENERGY_CONSERVE","HYPERACTIVE",    -0.35, 0.30}
};

#endif /* PESTINHA_NETWORK_DATA_H */
