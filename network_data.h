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
    {"TYPING", 4, 30.0, 0.15, 20.0},
    {"KEY_FAST", 4, 40.0, 0.20, 15.0},
    {"KEY_SLOW", 4, 15.0, 0.10, 30.0},
    {"IDLE", 4, 10.0, 0.05, 50.0},
    {"PAUSE", 4, 12.0, 0.05, 40.0},
    {"CMD_ENTER", 4, 50.0, 0.25, 10.0},
    {"BACKSPACE", 4, 35.0, 0.15, 20.0},
    {"DELETE", 4, 35.0, 0.15, 20.0},
    {"ARROW", 4, 25.0, 0.10, 25.0},
    {"TAB", 4, 20.0, 0.10, 25.0},
    {"SPACE", 4, 30.0, 0.12, 20.0},
    {"SHIFT", 4, 20.0, 0.08, 30.0},
    {"ALPHANUM", 4, 25.0, 0.10, 20.0},
    {"NUMERIC", 4, 25.0, 0.10, 20.0},
    {"SYMBOL", 4, 30.0, 0.12, 20.0},
    {"PUNCTUATION", 4, 20.0, 0.10, 25.0},

    {"CARESS", 4, 60.0, 0.30, 10.0},
    {"POKE", 4, 70.0, 0.40, 10.0},
    {"FEED", 4, 50.0, 0.25, 15.0},
    {"TALK", 4, 45.0, 0.20, 15.0},
    {"GREET", 4, 40.0, 0.18, 20.0},
    {"IGNORE", 4, 15.0, 0.05, 40.0},
    {"REWARD", 4, 65.0, 0.30, 10.0},
    {"PUNISH", 4, 65.0, 0.35, 10.0},
    {"HELP_REQ", 4, 30.0, 0.15, 25.0},
    {"CLEAN", 4, 35.0, 0.15, 20.0},
    {"PLAY", 4, 50.0, 0.22, 15.0},
    {"SLEEP_REQ", 4, 25.0, 0.10, 30.0},
    {"CALL", 4, 45.0, 0.20, 15.0},
    {"WHISTLE", 4, 40.0, 0.18, 20.0},
    {"MUSIC", 4, 35.0, 0.12, 25.0},
    {"NOISE", 4, 55.0, 0.25, 15.0},

    {"TIME_DAY", 4, 10.0, 0.02, 100.0},
    {"TIME_NIGHT", 4, 10.0, 0.02, 100.0},
    {"UPTIME", 4, 15.0, 0.05, 50.0},
    {"CPU_LOAD", 4, 20.0, 0.08, 30.0},
    {"IDLE_LONG", 4, 8.0, 0.02, 120.0},
    {"FOCUS_ACTIVE", 4, 25.0, 0.10, 25.0},
    {"SESSION_LEN", 4, 12.0, 0.04, 80.0},
    {"TICK_PULSE", 4, 20.0, 0.08, 30.0},
    {"TEMP_WARM", 4, 15.0, 0.05, 50.0},
    {"TEMP_COLD", 4, 15.0, 0.05, 50.0},
    {"USER_PRESENT", 4, 30.0, 0.10, 20.0},
    {"USER_AWAY", 4, 10.0, 0.03, 80.0},
    {"QUICK_CLICK", 4, 35.0, 0.15, 20.0},
    {"SLOW_CLICK", 4, 15.0, 0.05, 40.0},
    {"BURST_INPUT", 4, 50.0, 0.22, 15.0},
    {"RHYTHM_REGULAR", 4, 25.0, 0.10, 25.0},

    {"CIRCADIAN_PEAK", 4, 12.0, 0.04, 80.0},
    {"CIRCADIAN_LOW", 4, 12.0, 0.04, 80.0},
    {"FATIGUE_PULSE", 4, 18.0, 0.06, 40.0},
    {"HUNGER_PULSE", 4, 18.0, 0.06, 40.0},
    {"HEARTBEAT_FAST", 4, 30.0, 0.12, 20.0},
    {"HEARTBEAT_SLOW", 4, 15.0, 0.05, 50.0},
    {"BREATHING", 4, 20.0, 0.08, 30.0},
    {"ADRENALINE", 4, 45.0, 0.20, 15.0},
    {"DOPAMINE", 4, 50.0, 0.22, 15.0},
    {"SEROTONIN", 4, 25.0, 0.10, 25.0},
    {"CORTISOL", 4, 40.0, 0.18, 18.0},
    {"OXYTOCIN", 4, 35.0, 0.15, 20.0},
    {"ENDORPHIN", 4, 40.0, 0.18, 18.0},
    {"MEMORY_TRACE", 4, 20.0, 0.08, 30.0},
    {"STDP_SPIKE", 4, 30.0, 0.12, 20.0},
    {"REWARD_WAIT", 4, 22.0, 0.09, 25.0}
};

/* Definition of 32 Interneuron Pools (576 neurons total) */
static const InterPoolDef INTER_POOLS[PESTINHA_INTERNEURONS] = {
    {"ALERT", 18, "exc", 20.0, 0.05, 100.0},
    {"CURIOUS", 18, "exc", 22.0, 0.04, 120.0},
    {"CALM", 18, "exc", 25.0, 0.03, 150.0},
    {"PLAYFUL", 18, "exc", 18.0, 0.06, 90.0},
    {"FRUSTRATION", 18, "exc", 16.0, 0.07, 80.0},
    {"GLOOM", 18, "exc", 30.0, 0.02, 200.0},
    {"SLEEPY", 18, "exc", 35.0, 0.02, 220.0},
    {"HUNGRY", 18, "exc", 24.0, 0.04, 130.0},
    {"SATISFIED", 18, "exc", 26.0, 0.03, 140.0},
    {"BORED", 18, "exc", 28.0, 0.03, 160.0},
    {"SURPRISED", 18, "exc", 14.0, 0.08, 70.0},
    {"FEARFUL", 18, "exc", 15.0, 0.08, 75.0},
    {"AFFECTIONATE", 18, "exc", 22.0, 0.04, 110.0},
    {"LONELY", 18, "exc", 32.0, 0.02, 190.0},
    {"HYPERACTIVE", 18, "exc", 12.0, 0.10, 60.0},
    {"STUBBORN", 18, "exc", 20.0, 0.05, 120.0},
    {"CREATIVE", 18, "exc", 21.0, 0.05, 115.0},
    {"MEMORY_HOLD", 18, "exc", 40.0, 0.01, 300.0},
    {"REWARD_EXPECT", 18, "exc", 19.0, 0.06, 100.0},
    {"THREAT_DETECT", 18, "inh", 15.0, 0.08, 70.0},
    {"RHYTHM_GEN", 18, "exc", 18.0, 0.05, 90.0},
    {"FOCUS_POOL", 18, "exc", 23.0, 0.04, 130.0},
    {"COGNITIVE_LOW", 18, "inh", 30.0, 0.02, 180.0},
    {"COGNITIVE_HIGH", 18, "exc", 17.0, 0.07, 85.0},
    {"SOCIAL_SEEK", 18, "exc", 25.0, 0.04, 140.0},
    {"PAIN_REACTION", 18, "inh", 14.0, 0.09, 65.0},
    {"COMFORT_POOL", 18, "exc", 27.0, 0.03, 150.0},
    {"ENERGY_CONSERVE", 18, "exc", 35.0, 0.02, 210.0},
    {"HABITUATION_POOL", 18, "inh", 28.0, 0.03, 160.0},
    {"PLASTIC_NODE1", 18, "exc", 20.0, 0.05, 100.0},
    {"PLASTIC_NODE2", 18, "exc", 20.0, 0.05, 100.0},
    {"EXECUTIVE_CTRL", 18, "exc", 18.0, 0.06, 90.0}
};

/* Definition of 24 Motor Pools (192 neurons total) */
static const MotorPoolDef MOTOR_POOLS[PESTINHA_MOTORS] = {
    {"FACE_HAPPY", 8, 20.0, 0.05, 100.0},
    {"FACE_SAD", 8, 25.0, 0.04, 120.0},
    {"FACE_SLEEP", 8, 30.0, 0.02, 180.0},
    {"FACE_SURPRISED", 8, 15.0, 0.08, 70.0},
    {"FACE_ANGRY", 8, 16.0, 0.07, 80.0},
    {"FACE_CURIOUS", 8, 22.0, 0.05, 110.0},
    {"FACE_PLAYFUL", 8, 18.0, 0.06, 90.0},
    {"FACE_NEUTRAL", 8, 25.0, 0.03, 140.0},
    {"FACE_WINK", 8, 17.0, 0.07, 85.0},
    {"CHIRP_SOUND", 8, 14.0, 0.08, 65.0},
    {"BEEP_PULSE", 8, 15.0, 0.08, 70.0},
    {"PURR_EFFECT", 8, 24.0, 0.04, 130.0},
    {"WANDER_LEFT", 8, 20.0, 0.05, 100.0},
    {"WANDER_RIGHT", 8, 20.0, 0.05, 100.0},
    {"JUMP_ACTION", 8, 14.0, 0.09, 60.0},
    {"DANCE_ACTION", 8, 16.0, 0.07, 80.0},
    {"SLEEP_ACTION", 8, 35.0, 0.02, 200.0},
    {"EAT_ACTION", 8, 22.0, 0.05, 110.0},
    {"CUDDLE_ACTION", 8, 23.0, 0.04, 120.0},
    {"ALERT_EMIT", 8, 15.0, 0.08, 75.0},
    {"NOD_HEAD", 8, 18.0, 0.06, 90.0},
    {"SHAKE_HEAD", 8, 18.0, 0.06, 90.0},
    {"THINK_EMIT", 8, 21.0, 0.05, 105.0},
    {"TAIL_WAG", 8, 19.0, 0.06, 95.0}
};

/* Connectivity Map (Sample Connections across Pools) */
static const ConnDef DEFAULT_CONNS[] = {
    {"TYPING", "ALERT", 0.45, 0.35},
    {"KEY_FAST", "HYPERACTIVE", 0.50, 0.40},
    {"IDLE", "SLEEPY", 0.40, 0.30},
    {"CARESS", "PLAYFUL", 0.60, 0.50},
    {"CARESS", "AFFECTIONATE", 0.65, 0.55},
    {"POKE", "FRUSTRATION", 0.60, 0.45},
    {"POKE", "SURPRISED", 0.55, 0.40},
    {"FEED", "SATISFIED", 0.70, 0.60},
    {"FEED", "CALM", 0.50, 0.40},
    {"REWARD", "DOPAMINE", 0.75, 0.65},
    {"PUNISH", "CORTISOL", 0.70, 0.60},
    {"ALERT", "FACE_SURPRISED", 0.50, 0.40},
    {"PLAYFUL", "FACE_PLAYFUL", 0.60, 0.50},
    {"PLAYFUL", "DANCE_ACTION", 0.55, 0.45},
    {"CALM", "FACE_HAPPY", 0.50, 0.40},
    {"SLEEPY", "FACE_SLEEP", 0.65, 0.55},
    {"SLEEPY", "SLEEP_ACTION", 0.70, 0.60},
    {"FRUSTRATION", "FACE_ANGRY", 0.60, 0.50},
    {"GLOOM", "FACE_SAD", 0.55, 0.45},
    {"AFFECTIONATE", "PURR_EFFECT", 0.60, 0.50},
    {"CURIOUS", "FACE_CURIOUS", 0.55, 0.45},
    {"CURIOUS", "WANDER_LEFT", 0.35, 0.30},
    {"CURIOUS", "WANDER_RIGHT", 0.35, 0.30}
};

#endif /* PESTINHA_NETWORK_DATA_H */
