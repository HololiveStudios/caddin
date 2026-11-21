#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "config.h"
#include "window.h"
#include "face.h"
#include "animation.h"
#include "softbody.h"
#include "emotions.h"
#include "autonomy.h"

/* motor SNN — headers no root do projeto */
#include "neuro.h"
#include "homeostasis.h"
#include "behavior.h"
#include "conditioning.h"
#include "memory.h"
#include "sensors.h"
#include "runtime_types.h"

/* novos módulos de biologia, mente e percepção */
#include "core/fixed.h"
#include "core/arena.h"
#include "core/rng.h"
#include "core/neuromod.h"
#include "core/cpg.h"
#include "core/predict.h"
#include "core/ontogeny.h"
#include "core/config_rules.h"
#include "core/sdm.h"
#include "core/rituals.h"
#include "core/event_ring.h"
#include "core/bandit.h"
#include "core/daily_summary.h"
#include "core/telemetry_fifo.h"
#include "sense/sense_fatigue.h"
#include "sense/sense_keystroke_biometrics.h"
#include "sense/sense_mouse.h"
#include "sense/sense_scroll.h"
#include "sense/sense_windows.h"
#include "sense/sense_sysload.h"
#include "sense/sense_luma.h"
#include "sense/sense_heatmap.h"
#include "body/palette.h"
#include "body/voice_synth.h"
#include "body/audio_out.h"
#include "mind/workspace.h"
#include "lang/lexicon.h"
#include "behavior/behavior_dmn.h"
#include "behavior/behavior_bond.h"
#include "behavior/behavior_explore.h"
#include "behavior/behavior_hide_hunt.h"
#include "behavior/behavior_perch.h"
#include "behavior/behavior_startle_relief.h"
#include "behavior/behavior_dance.h"
#include "sense/sense_mic.h"
#include "sense/sense_music.h"
#include "sense/sense_text.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <signal.h>

#if defined(__linux__) && defined(__GLIBC__)
#  include <malloc.h>
#endif

#if defined(__linux__) && !defined(_WIN32) && !defined(CADDIN_HEADLESS)
extern void *x11_get_display(void);
#endif

static volatile sig_atomic_t g_running = 1;
static void handle_sig(int sig) {
    (void)sig;
    g_running = 0;
}

#if defined(_WIN32)
#  include <windows.h>
static void sleep_ms(int ms) { Sleep(ms); }
static double now_seconds(void) {
    static LARGE_INTEGER freq;
    static int initialized = 0;
    if (!initialized) {
        QueryPerformanceFrequency(&freq);
        initialized = 1;
    }
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return (double)count.QuadPart / (double)freq.QuadPart;
}
#elif defined(__unix__) || defined(__APPLE__)
#  include <unistd.h>
static void sleep_ms(int ms) {
    struct timespec ts = { ms/1000, (long)(ms%1000)*1000000L };
    nanosleep(&ts, NULL);
}
static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}
#else
static void sleep_ms(int ms) { (void)ms; }
static double now_seconds(void) { return (double)clock() / CLOCKS_PER_SEC; }
#endif

/* Caminho do arquivo de persistência sináptica */
#define BRAIN_FILE        "caddin.brain"
#define BRAIN_FILE_LEGACY "cortex_pet.brain"
#define ONTO_FILE         "caddin.onto"
#define SDM_FILE          "caddin.sdm"

/* Intervalo de auto-save (em segundos) */
#define AUTOSAVE_INTERVAL 60.0

static void sdm_save_state(const SparseDistributedMemory *sdm, const char *path) {
    if (!sdm || !path) return;
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (!f) return;

    static uint8_t sdm_buf[sizeof(sdm->counters) + 64];
    size_t sz = sdm_serialize(sdm, sdm_buf, sizeof(sdm_buf));
    if (sz > 0) {
        fwrite(sdm_buf, 1, sz, f);
    }
    fflush(f);
    fclose(f);

#ifdef _WIN32
    MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
#else
    rename(tmp, path);
#endif
}

static bool sdm_load_state(SparseDistributedMemory *sdm, const char *path) {
    if (!sdm || !path) return false;
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    static uint8_t sdm_buf[sizeof(sdm->counters) + 64];
    size_t read_bytes = fread(sdm_buf, 1, sizeof(sdm_buf), f);
    fclose(f);

    return sdm_deserialize(sdm, sdm_buf, read_bytes);
}

#define BANDIT_FILE "caddin.bandit"
#define KEYS_FILE   "caddin.keys"
#define DAILY_LOG   "caddin_daily.log"

static void keys_save_state(const SenseKeystrokeBiometrics *kb, const char *path) {
    if (!kb || !path) return;
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (!f) return;

    static uint8_t buf[256];
    size_t sz = sense_keystroke_serialize(kb, buf, sizeof(buf));
    if (sz > 0) {
        fwrite(buf, 1, sz, f);
    }
    fflush(f);
    fclose(f);

#ifdef _WIN32
    MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
#else
    rename(tmp, path);
#endif
}

static bool keys_load_state(SenseKeystrokeBiometrics *kb, const char *path) {
    if (!kb || !path) return false;
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    static uint8_t buf[256];
    size_t read_bytes = fread(buf, 1, sizeof(buf), f);
    fclose(f);
    if (read_bytes == 0) return false;

    return sense_keystroke_deserialize(kb, buf, read_bytes);
}

static void bandit_save_state(const DopaminergicBandit *b, const char *path) {
    if (!b || !path) return;
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (!f) return;

    static uint8_t buf[256];
    size_t sz = bandit_serialize(b, buf, sizeof(buf));
    if (sz > 0) {
        fwrite(buf, 1, sz, f);
    }
    fflush(f);
    fclose(f);

#ifdef _WIN32
    MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
#else
    rename(tmp, path);
#endif
}

static bool bandit_load_state(DopaminergicBandit *b, const char *path) {
    if (!b || !path) return false;
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    static uint8_t buf[256];
    size_t read_bytes = fread(buf, 1, sizeof(buf), f);
    fclose(f);

    return bandit_deserialize(b, buf, read_bytes);
}

static void onto_save_state(const Ontogeny *o, const Imprint *im, const char *path) {
    if (!o || !im || !path) return;
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (!f) return;

    static uint8_t onto_buf[32768];
    static uint8_t impr_buf[64];
    size_t onto_sz = onto_serialize(o, onto_buf, sizeof(onto_buf));
    size_t impr_sz = imprint_serialize(im, impr_buf, sizeof(impr_buf));

    uint32_t magic = 0x4F4E544F; /* "ONTO" */
    uint32_t sz1 = (uint32_t)onto_sz;
    uint32_t sz2 = (uint32_t)impr_sz;

    fwrite(&magic, 4, 1, f);
    fwrite(&sz1, 4, 1, f);
    if (onto_sz > 0) fwrite(onto_buf, 1, onto_sz, f);
    fwrite(&sz2, 4, 1, f);
    if (impr_sz > 0) fwrite(impr_buf, 1, impr_sz, f);

    fflush(f);
    fclose(f);

#ifdef _WIN32
    MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
#else
    rename(tmp, path);
#endif
}

static bool onto_load_state(Ontogeny *o, Imprint *im, const char *path) {
    if (!o || !im || !path) return false;
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    uint32_t magic = 0, sz1 = 0, sz2 = 0;
    if (fread(&magic, 4, 1, f) != 1 || magic != 0x4F4E544F) {
        fclose(f);
        return false;
    }
    if (fread(&sz1, 4, 1, f) != 1 || sz1 > 65536) {
        fclose(f);
        return false;
    }
    static uint8_t onto_buf[65536];
    if (fread(onto_buf, 1, sz1, f) != sz1) {
        fclose(f);
        return false;
    }
    if (fread(&sz2, 4, 1, f) != 1 || sz2 > 256) {
        fclose(f);
        return false;
    }
    static uint8_t impr_buf[256];
    if (fread(impr_buf, 1, sz2, f) != sz2) {
        fclose(f);
        return false;
    }
    fclose(f);

    bool ok1 = (onto_deserialize(o, onto_buf, sz1) == 1);
    bool ok2 = (imprint_deserialize(im, impr_buf, sz2) == 1);
    return ok1 && ok2;
}

typedef struct {
    Neuromodulators *neuromod;
    SoftBody        *sb;
    VoiceSynth      *voice;
    Drives          *drives;
    double           now_s;
} RuleContext;

static void handle_rule_action(const char *action, float param, void *ctx) {
    RuleContext *rc = (RuleContext *)ctx;
    if (!rc || !action) return;

    if (strcmp(action, "inject_da") == 0) {
        neuromod_inject_da(rc->neuromod, fix_from_float(param > 0 ? param : 0.1f));
    } else if (strcmp(action, "inject_ne") == 0) {
        neuromod_inject_ne(rc->neuromod, fix_from_float(param > 0 ? param : 0.1f));
    } else if (strcmp(action, "inject_cort") == 0) {
        neuromod_inject_cort(rc->neuromod, fix_from_float(param > 0 ? param : 0.1f));
    } else if (strcmp(action, "inject_ser") == 0) {
        neuromod_inject_ser(rc->neuromod, fix_from_float(param > 0 ? param : 0.1f));
    } else if (strcmp(action, "react_happy") == 0) {
        neuromod_inject_da(rc->neuromod, fix_from_float(0.15f));
        softbody_jiggle(rc->sb, 0.35f);
    } else if (strcmp(action, "react_sad") == 0) {
        neuromod_inject_cort(rc->neuromod, fix_from_float(0.12f));
        softbody_squish(rc->sb, 0.30f);
    } else if (strcmp(action, "react_curious") == 0) {
        neuromod_inject_ne(rc->neuromod, fix_from_float(0.08f));
        softbody_jiggle(rc->sb, 0.25f);
    }
}

static void record_episode_sdm(SparseDistributedMemory *sdm, const char *win_title,
                               int hour, double valence, double stress, int event_type) {
    if (!sdm) return;
    EpisodeDescriptor ep;
    memset(&ep, 0, sizeof(ep));
    snprintf(ep.window_name, sizeof(ep.window_name), "%s", win_title ? win_title : "");
    ep.hour = (hour >= 0 && hour < 24) ? hour : 12;
    ep.valence_level = (int)round(valence * 2.0);
    if (ep.valence_level > 2) ep.valence_level = 2;
    if (ep.valence_level < -2) ep.valence_level = -2;
    ep.stress_level = (int)round(stress * 3.0);
    if (ep.stress_level > 3) ep.stress_level = 3;
    if (ep.stress_level < 0) ep.stress_level = 0;
    ep.event_type = event_type;

    Hypervector vec = sdm_encode_episode(&ep);
    sdm_store(sdm, vec, vec);
}

typedef struct {
    VoiceSynth      *voice;
    SoftBody        *sb;
    Neuromodulators *neuromod;
    Drives          *drives;
    double           now_s;
} MainRitualContext;

static void on_milestone_reached(uint32_t hours, void *ctx) {
    MainRitualContext *rc = (MainRitualContext *)ctx;
    if (!rc) return;
    printf("[RITUAL] 🎉 Marco alcançado: %u hora(s) de convivência com o Caddin!\n", hours);
    neuromod_inject_da(rc->neuromod, fix_from_float(0.35f));
    neuromod_inject_ser(rc->neuromod, fix_from_float(0.20f));
    rc->drives->affection = 1.0;
    softbody_jiggle(rc->sb, 0.40f);
    voice_synth_trigger_ex(rc->voice, VOICE_TRILL, 0.50f, 0.95f);
}

static void on_circadian_greeting(bool is_morning, void *ctx) {
    MainRitualContext *rc = (MainRitualContext *)ctx;
    if (!rc) return;
    if (is_morning) {
        printf("[RITUAL] ☀️ Bom dia! Caddin despertando animado.\n");
        neuromod_inject_da(rc->neuromod, fix_from_float(0.15f));
        softbody_jiggle(rc->sb, 0.25f);
        voice_synth_trigger_ex(rc->voice, VOICE_CHIRP_HAPPY, 0.22f, 0.8f);
    } else {
        printf("[RITUAL] 🌙 Boa noite! Caddin se preparando para repouso.\n");
        neuromod_inject_ser(rc->neuromod, fix_from_float(0.15f));
        softbody_squish(rc->sb, 0.20f);
        voice_synth_trigger_ex(rc->voice, VOICE_GRUNT_IDLE, 0.40f, 0.5f);
    }
}

/* ── Índices dos motores usados para efeitos físicos ── */
enum {
    M_FACE_HAPPY = 0, M_FACE_SAD, M_FACE_SLEEP, M_FACE_SURPRISED,
    M_FACE_ANGRY, M_FACE_CURIOUS, M_FACE_PLAYFUL, M_FACE_NEUTRAL,
    M_FACE_WINK,
    M_CHIRP_SOUND, M_BEEP_PULSE, M_PURR_EFFECT,
    M_WANDER_LEFT, M_WANDER_RIGHT, M_JUMP_ACTION, M_DANCE_ACTION,
    M_SLEEP_ACTION, M_EAT_ACTION, M_CUDDLE_ACTION, M_ALERT_EMIT,
    M_NOD_HEAD, M_SHAKE_HEAD, M_THINK_EMIT, M_TAIL_WAG
};

#ifdef ANDROID_APP
int caddin_app_main(int argc, char **argv) {
    (void)argc; (void)argv;
#else
int main(void) {
#endif
#if defined(__linux__) && defined(__GLIBC__)
    mallopt(M_TRIM_THRESHOLD, 64 * 1024);
    mallopt(M_MMAP_THRESHOLD, 64 * 1024);
#endif
    srand((unsigned)time(NULL));
    signal(SIGINT, handle_sig);
    signal(SIGTERM, handle_sig);

    /* ── janela e renderer nativo 2D (X11 / Win32) ── */
    PetWindow *win = window_create("Caddin", WINDOW_W, WINDOW_H);
    if (!win) { fprintf(stderr, "window_create falhou\n"); return 1; }

    /* ── motor SNN ── */
    Brain *brain = brain_create(42, "default");
    if (!brain) { fprintf(stderr, "brain_create falhou\n"); return 1; }

    /* ── Persistência: carrega pesos sinápticos anteriores ── */
    if (brain_load(brain, BRAIN_FILE)) {
        printf("[SNN] Memória sináptica carregada: %s\n", BRAIN_FILE);
    } else if (brain_load(brain, BRAIN_FILE_LEGACY)) {
        printf("[SNN] Memória sináptica carregada (legada): %s\n", BRAIN_FILE_LEGACY);
    } else {
        printf("[SNN] Tabula Rasa: inicializando nova matriz sináptica.\n");
    }

    /* ── Ontogenia & Imprinting ── */
    static Ontogeny ontogeny;
    static Imprint  imprint;
    imprint_reset(&imprint);

    if (onto_load_state(&ontogeny, &imprint, ONTO_FILE)) {
        printf("[ONTO] Estado ontogenético carregado: %s (Estágio: %d, Vigília: %us, Ciclos de sono: %u)\n",
               ONTO_FILE, ontogeny.stage, ontogeny.awake_s, ontogeny.sleeps);
    } else {
        onto_init(&ontogeny, NULL);
        printf("[ONTO] Ontogenia inicializada (Filhote, plasticidade plena)\n");
    }

    /* ── Regras Customizáveis (.caddinrc) e Privacidade ── */
    ConfigRules config_rules;
    config_rules_init(&config_rules);
    if (config_rules_load(&config_rules, ".caddinrc")) {
        printf("[CONFIG] Regras ativas (.caddinrc): %zu regras, %zu apps ignorados.\n",
               config_rules.num_rules, config_rules.num_ignored);
    } else if (config_rules_load(&config_rules, "caddin.conf")) {
        printf("[CONFIG] Regras ativas (caddin.conf): %zu regras, %zu apps ignorados.\n",
               config_rules.num_rules, config_rules.num_ignored);
    }

    Drives          drives;
    Emotion         emotion;
    Behavior        behavior;
    ConditioningState cond;
    EpisodeMemory   memory;
    Sensors        *sensors = sensors_open();

    homeostasis_init(&drives);
    behavior_init(&behavior, &emotion);
    conditioning_init(&cond);
    memory_init(&memory);

    /* ── Memória Episódica Hiperdimensional (SDM Kanerva) ── */
    static SparseDistributedMemory sdm_memory;
    sdm_init(&sdm_memory, 0x1337BEEF);
    if (sdm_load_state(&sdm_memory, SDM_FILE)) {
        printf("[SDM] Memória episódica hiperdimensional carregada: %s\n", SDM_FILE);
    } else {
        printf("[SDM] Memória episódica inicializada (Tabula Rasa distribuída)\n");
    }

    /* ── Elegibilidade Estendida, Bandit Dopaminérgico e Fadiga ── */
    EventRingBuffer event_ring;
    event_ring_init(&event_ring);

    DopaminergicBandit bandit;
    bandit_init(&bandit);
    if (bandit_load_state(&bandit, BANDIT_FILE)) {
        printf("[BANDIT] Modelo de intervenção carregado: %s (%u intervenções registradas)\n",
               BANDIT_FILE, bandit.total_pulls);
    } else {
        printf("[BANDIT] Aprendizado por reforço inicializado (Bandit UCB1)\n");
    }

    SenseFatigue sense_fatigue;
    sense_fatigue_init(&sense_fatigue);

    /* ── Fase 6: Biometria de Teclado, Resumo Diário e Telemetria (100% C) ── */
    SenseKeystrokeBiometrics sense_keystroke;
    sense_keystroke_init(&sense_keystroke);
    if (keys_load_state(&sense_keystroke, KEYS_FILE)) {
        printf("[KEYSTROKE] Biometria do dono carregada: %s (%u amostras de ritmo)\n",
               KEYS_FILE, sense_keystroke.owner_samples);
    } else {
        printf("[KEYSTROKE] Biometria inicializada (Aguardando aprendizado do ritmo de digitação)\n");
    }

    time_t raw_init_t = time(NULL);
    struct tm *lt_init = localtime(&raw_init_t);
    int start_day = lt_init ? lt_init->tm_yday : 0;

    DailySummary daily_summary;
    daily_summary_init(&daily_summary, start_day);

    TelemetryFifo telemetry_fifo;
    telemetry_fifo_init(&telemetry_fifo);

    /* ── novos subsistemas: biologia, percepção e mente ── */
    SenseMouse              sense_mouse;
    SenseScroll             sense_scroll;
    SenseWindows            sense_windows;
    SenseSysload            sense_sysload;
    SenseHeatmap            sense_heatmap;
    Neuromodulators         neuromod;
    CentralPatternGenerator cpg;
    TemporalPredictor       predictor;
    GlobalWorkspace         workspace;
    Lexicon                 lexicon;
    VoiceSynth              voice;
    BehaviorDmn             dmn;
    BehaviorBond            bond;
    BehaviorExplore         explore;
    BehaviorHideHunt        hide_hunt;
    BehaviorPerch           perch;
    BehaviorStartleRelief   startle_relief;
    SenseMic                sense_mic;
    SenseMusic              sense_music;
    SenseText               sense_text;
    BehaviorDance           dance;

    sense_mouse_init(&sense_mouse);
    sense_scroll_init(&sense_scroll);
    sense_windows_init(&sense_windows);
    sense_sysload_init(&sense_sysload);
    neuromod_init(&neuromod);
    cpg_init(&cpg);
    predict_init(&predictor);
    workspace_init(&workspace);
    lexicon_init(&lexicon);
    voice_synth_init(&voice, 16000);
    behavior_dmn_init(&dmn);
    behavior_bond_init(&bond);
    behavior_explore_init(&explore);
    behavior_hide_hunt_init(&hide_hunt);
    behavior_perch_init(&perch);
    behavior_startle_relief_init(&startle_relief);
    behavior_dance_init(&dance);
    sense_mic_init(&sense_mic, 16000);
    sense_music_init(&sense_music, 16000);
    sense_text_init(&sense_text);

    /* ── Luma precisa de Display*, criado dinamicamente ── */
    SenseLuma *sense_luma = NULL;
#if defined(__linux__) && !defined(_WIN32) && !defined(CADDIN_HEADLESS)
    {
        /* Inclui X11 apenas no bloco guardado — evita exposição do tipo Display
         * ao compilador do main.c que não inclui Xlib diretamente */
        void *dpy2 = NULL;
        unsigned long root2 = 0;
        {
            /* x11_get_display retorna struct _XDisplay* (opaco fora de Xlib),
             * mas sense_luma_create o aceita como void* */
            extern unsigned long x11_get_root(void);
            dpy2  = x11_get_display();
            root2 = x11_get_root();
        }
        if (dpy2) sense_luma = sense_luma_create(dpy2, root2);
    }
#endif

    /* ── Mapa de calor: dimensões da tela ── */
    {
        int wa_x, wa_y, wa_w, wa_h;
        window_get_workarea(&wa_x, &wa_y, &wa_w, &wa_h);
        sense_heatmap_init(&sense_heatmap, wa_w > 0 ? wa_w : 1920,
                                           wa_h > 0 ? wa_h : 1080);
    }

    /* ── Áudio (Fix 1) ── */
    audio_out_init(16000);

    /* ── animação, soft-body e emoções reativas ── */
    AnimState anim;
    anim_init(&anim);

    SoftBody sb;
    softbody_init(&sb);

    EmotionMachine em;
    emot_init(&em);

    AutonomyState autonomy;
    autonomy_init(&autonomy);

    /* ── Rituais de convivência e calibração de maturação ontogenética ── */
    RitualState rituals;
    rituals_init(&rituals);
    voice_synth_set_maturity_stage(&voice, ontogeny.stage);
    softbody_set_maturity_stage(&sb, ontogeny.stage);

    static Canvas canvas;
    int16_t voice_pcm[512];

    /* Cooldowns de voz — impedem re-trigger enquanto o som anterior ainda toca.
     * Cada slot guarda o timestamp do último trigger para aquele tipo de voz. */
    double voice_last_purr   = -99.0;
    double voice_last_chirp  = -99.0;
    double voice_last_alarm  = -99.0;
    double voice_last_trill  = -99.0;
    double voice_last_whine  = -99.0;
    double voice_last_growl  = -99.0;
    double voice_last_grunt  = -99.0;
    /* Duração mínima entre triggers (segundos) */
    #define VOICE_COOLDOWN_PURR   3.0   /* ronron: não repete antes de 3s         */
    #define VOICE_COOLDOWN_CHIRP  1.5   /* pio curioso: re-ativa a cada 1.5s       */
    #define VOICE_COOLDOWN_ALARM  2.0   /* squeak/susto: cooldown de 2s            */
    #define VOICE_COOLDOWN_TRILL  4.0   /* trilo de alegria: espaçado              */
    #define VOICE_COOLDOWN_WHINE  5.0   /* reclamação/saudade: pouco frequente     */
    #define VOICE_COOLDOWN_GROWL  2.5   /* growl raiva: cooldown moderado          */
    #define VOICE_COOLDOWN_GRUNT 20.0   /* bocejo: raro e orgânico                 */

    const double TARGET_DT = 1.0 / 60.0;
    double t_last = now_seconds();
    double sim_t  = 0.0;
    double last_save_t = 0.0;
    double sleep_accum_s = 0.0;
    uint32_t current_tick = 0;

    while (g_running && !window_should_close(win)) {

        double t_now = now_seconds();
        double dt    = t_now - t_last;
        if (dt > 0.1) dt = 0.1;
        t_last = t_now;
        sim_t += dt;
        current_tick++;

        window_poll_events(win);
        window_update_physics(win, dt);

        bool bumped   = window_was_bumped(win);
        bool clicked  = window_was_clicked(win);
        bool dragging = window_is_dragging(win);
        double speed  = window_get_speed(win);
        double vx     = window_get_vx(win);
        double vy     = window_get_vy(win);

        int pet_x = 0, pet_y = 0;
        window_get_pos(win, &pet_x, &pet_y);

        int cursor_x = 0, cursor_y = 0;
#if defined(__linux__) && !defined(_WIN32) && !defined(CADDIN_HEADLESS)
        {
            extern bool x11_get_screen_cursor(int *out_x, int *out_y);
            x11_get_screen_cursor(&cursor_x, &cursor_y);
        }
#endif

        /* ══════════════════════════════════════════════════
         * CAMADA 1: Percepção SO Avançada & Sensores
         * ══════════════════════════════════════════════════ */

        /* Fix 2: centro real do pet (janela 192×144, centro = +96, +72) */
        int pet_cx = pet_x + WINDOW_W / 2;
        int pet_cy = pet_y + WINDOW_H / 2;
        sense_mouse_update(&sense_mouse, cursor_x, cursor_y, pet_cx, pet_cy, 48);
        sense_fatigue_register_mouse(&sense_fatigue, cursor_x, cursor_y, sim_t);

        /* Fix 3: scroll wheel */
        {
            int wheel = window_get_wheel(win);
            if (wheel != 0) {
                sense_scroll_register(&sense_scroll, wheel);
            }
        }
        sense_scroll_step(&sense_scroll);

        /* Fix 4: janela ativa — detecção de zona de perigo e troca de foco */
        int aw_x = 0, aw_y = 0, aw_ww = 0, aw_hh = 0;
        uint64_t aw_id = 0;
        if (window_get_active_window_rect(win, &aw_x, &aw_y, &aw_ww, &aw_hh, &aw_id)) {
            sense_windows_update(&sense_windows, aw_id,
                                 aw_x, aw_y, aw_ww, aw_hh);
            if (sense_windows_did_switch_focus(&sense_windows)) {
                neuromod_inject_ne(&neuromod, fix_from_float(0.05f));
                softbody_jiggle(&sb, 0.18f);
            }
        }

        /* Heatmap do cursor */
        sense_heatmap_register(&sense_heatmap, cursor_x, cursor_y);
        sense_heatmap_decay(&sense_heatmap, t_now);

        /* Carga do sistema */
        if (sense_sysload_update(&sense_sysload, t_now)) {
            fix16_t cpu_load = sense_sysload_get_cpu(&sense_sysload);
            fix16_t cpu_temp = sense_sysload_get_temp(&sense_sysload);
            /* CPU quente → calor ambiental → cortisol leve */
            if (cpu_temp > fix_from_float(0.65f)) {
                neuromod_inject_cort(&neuromod, fix_from_float(0.01f));
            }
            /* CPU sobrecarregada → pet sente "aperto" */
            if (cpu_load > fix_from_float(0.8f)) {
                softbody_squish(&sb, 0.05f);
            }
        }

        /* Luminosidade sob o pet */
        if (sense_luma) {
            fix16_t luma = sense_luma_sample(sense_luma, pet_cx, pet_cy, 16);
            /* Escuro embaixo → dormência mais provável */
            if (luma < fix_from_float(0.2f)) {
                neuromod_inject_ser(&neuromod, fix_from_float(0.005f));
            }
        }

        /* Detecção de quina perigosa (botão fechar) */
        if (sense_windows_is_near_danger_zone(&sense_windows, pet_x, pet_y, 45)) {
            neuromod_inject_ne(&neuromod, fix_from_float(0.04f));
            neuromod_inject_cort(&neuromod, fix_from_float(0.02f));
            softbody_stretch(&sb, 0.10f);
        }

        /* Detecção de atrito e carinho suave */
        if (sense_mouse_is_rubbing(&sense_mouse)) {
            neuromod_inject_da(&neuromod, fix_from_float(0.03f));
            neuromod_inject_ser(&neuromod, fix_from_float(0.02f));
            workspace_submit_candidate(&workspace, STIMULUS_MOUSE_TOUCH, fix_from_float(1.2f), current_tick);
            drives.affection = (drives.affection < 0.98) ? (drives.affection + 0.005) : 1.0;
            behavior_bond_on_gentle_touch(&bond);
            behavior_dmn_signal_activity(&dmn, t_now);

            event_ring_push(&event_ring, EV_USER_TOUCH, current_tick, sim_t, 1.0f, "caress");
            daily_summary_record_touch(&daily_summary);
            if (sim_t - bandit.last_pull_time < 60.0 && bandit.last_arm_chosen >= 0) {
                bandit_update_reward(&bandit, (BanditArm)bandit.last_arm_chosen, 1.0f);
            }
        }

        /* Reflexo Nociceptivo Espinhal (Sacudida violenta) */
        fix16_t shake_intensity = sense_mouse_get_shake_intensity(&sense_mouse);
        fix16_t motor_jolt = 0;
        if (neuromod_check_nociceptive_reflex(&neuromod, shake_intensity, &motor_jolt)) {
            softbody_squish(&sb, 0.45f);
            behavior_startle_relief_trigger(&startle_relief, (float)fix_to_float(shake_intensity));
            if (t_now - voice_last_alarm > VOICE_COOLDOWN_ALARM) {
                /* Sacudida violenta → susto agudo instantâneo */
                voice_synth_trigger_ex(&voice, VOICE_SQUEAK_STARTLED, 0.12f, 0.9f);
                voice_last_alarm = t_now;
            }
            workspace_submit_candidate(&workspace, STIMULUS_VIOLENT_SHAKE, fix_from_float(2.5f), current_tick);
            lexicon_observe_word(&lexicon, "susto", fix_from_float(-0.9f), fix_from_float(0.9f), 2);
            drives.stress = (drives.stress < 0.95) ? (drives.stress + 0.15) : 1.0;
            behavior_bond_on_shake(&bond);
            behavior_dmn_signal_activity(&dmn, t_now);

            event_ring_push(&event_ring, EV_USER_SHAKE, current_tick, sim_t, 1.0f, "shake");
            daily_summary_record_shake(&daily_summary);
            if (sim_t - bandit.last_pull_time < 30.0 && bandit.last_arm_chosen >= 0) {
                bandit_update_reward(&bandit, (BanditArm)bandit.last_arm_chosen, -1.0f);
            }
        }

        /* ── Percepção Avançada: Microfone (Som ambiente / palmas) ── */
        sense_mic_update(&sense_mic, (float)dt);
        if (sense_mic_is_loud(&sense_mic)) {
            behavior_startle_relief_trigger(&startle_relief, 0.65f);
            drives.stress = (drives.stress < 0.92) ? (drives.stress + 0.08) : 1.0;
            workspace_submit_candidate(&workspace, STIMULUS_SPOKEN_WORD, fix_from_float(1.5f), current_tick);
            behavior_dmn_signal_activity(&dmn, t_now);
        }

        /* ── Percepção Avançada: Leitura de Texto Contextual da Janela Ativa ── */
        sense_text_update(&sense_text, (float)dt,
#if defined(__linux__) && !defined(_WIN32) && !defined(CADDIN_HEADLESS)
                          x11_get_display(),
#else
                          NULL,
#endif
                          (unsigned long)aw_id);

        const char *current_win_title = sense_text_get_title(&sense_text);
        bool is_win_ignored = config_rules_is_ignored(&config_rules, current_win_title);

        if (!is_win_ignored) {
            if (sense_text_has_new_word(&sense_text)) {
                lexicon_observe_word(&lexicon, sense_text_get_last_word(&sense_text),
                                     fix_from_float((float)emotion.valence),
                                     fix_from_float((float)emotion.arousal), 1);
            }

            /* Reação a Erros na Tela (Frustração / Alerta) */
            if (sense_text_has_error(&sense_text)) {
                neuromod_inject_ne(&neuromod, fix_from_float(0.12f));
                neuromod_inject_cort(&neuromod, fix_from_float(0.08f));
                softbody_squish(&sb, 0.35f);
                if (t_now - voice_last_whine > VOICE_COOLDOWN_WHINE) {
                    voice_synth_trigger_ex(&voice, VOICE_WHINE_PROTEST, 0.30f, 0.7f);
                    voice_last_whine = t_now;
                }
                workspace_submit_candidate(&workspace, STIMULUS_SURPRISE_MISMATCH, fix_from_float(1.6f), current_tick);
                drives.stress = (drives.stress < 0.90) ? (drives.stress + 0.10) : 1.0;
                record_episode_sdm(&sdm_memory, current_win_title, 12, emotion.valence, drives.stress, 3);
                event_ring_push(&event_ring, EV_CODE_ERROR, current_tick, sim_t, 1.0f, "error");
                daily_summary_record_code_error(&daily_summary);
            }

            /* Reação a Sucessos / Build Passed na Tela (Celebração) */
            if (sense_text_has_success(&sense_text)) {
                neuromod_inject_da(&neuromod, fix_from_float(0.18f));
                neuromod_inject_ser(&neuromod, fix_from_float(0.10f));
                softbody_jiggle(&sb, 0.38f);
                if (t_now - voice_last_chirp > VOICE_COOLDOWN_CHIRP) {
                    voice_synth_trigger_ex(&voice, VOICE_CHIRP_HAPPY, 0.25f, 0.85f);
                    voice_last_chirp = t_now;
                }
                drives.affection = (drives.affection < 0.98) ? (drives.affection + 0.02) : 1.0;
                drives.energy = (drives.energy < 0.95) ? (drives.energy + 0.05) : 1.0;
                record_episode_sdm(&sdm_memory, current_win_title, 12, emotion.valence, drives.stress, 4);
                event_ring_push(&event_ring, EV_CODE_SUCCESS, current_tick, sim_t, 1.0f, "success");
                daily_summary_record_code_success(&daily_summary);
            }
        }

        /* ── Resposta Bifásica Susto → Alívio ── */
        float squish_relief = 0.0f, ser_boost = 0.0f;
        if (behavior_startle_relief_update(&startle_relief, (float)dt, &squish_relief, &ser_boost)) {
            /* Transição para Fase 2: Alívio progressivo e suspiro */
            neuromod_inject_ser(&neuromod, fix_from_float(ser_boost));
            if (t_now - voice_last_grunt > 4.0) {
                voice_synth_trigger_ex(&voice, VOICE_GRUNT_IDLE, 0.35f, 0.5f);
                voice_last_grunt = t_now;
            }
        }
        if (squish_relief != 0.0f) {
            softbody_squish(&sb, squish_relief * 0.4f);
        }

        /* ── Comportamentos Avançados: Se Esconder (Hide) / Caçar (Hunt) ── */
        float hh_dx = 0.0f, hh_dy = 0.0f;
        behavior_hide_hunt_update(&hide_hunt, (float)dt, (float)pet_x, (float)pet_y,
                                  (float)cursor_x, (float)cursor_y,
                                  aw_x, aw_y, aw_ww, aw_hh,
                                  (float)drives.stress, (float)drives.energy, (float)drives.curiosity,
                                  &hh_dx, &hh_dy);
        if (hh_dx != 0.0f || hh_dy != 0.0f) {
            pet_x += hh_dx;
            pet_y += hh_dy;
        }

        /* ── Comportamento Avançado: Pouso no Galho/Topo da Janela (Perch) ── */
        float perch_dx = 0.0f, perch_dy = 0.0f;
        if (behavior_perch_update(&perch, (float)dt, (float)pet_x, (float)pet_y,
                                  aw_x, aw_y, aw_ww, aw_hh,
                                  (float)drives.energy, (float)drives.stress,
                                  &perch_dx, &perch_dy)) {
            pet_y += perch_dy;
            if (perch_dx != 0.0f) pet_x += perch_dx;
        }

        /* Colisão com borda */
        if (bumped) {
            softbody_squish(&sb, 0.38f);
        }

        /* Clique direto */
        if (clicked) {
            softbody_jiggle(&sb, 0.40f);
            fix16_t reward_da = 0;
            predict_register_event(&predictor, current_tick, &reward_da);
            if (reward_da > 0) {
                neuromod_inject_da(&neuromod, reward_da);
            }
            behavior_dmn_signal_activity(&dmn, t_now);
            behavior_bond_on_gentle_touch(&bond);
        }

        /* Predição Temporal: Mismatch Spikes (Surpresa) */
        fix16_t surprise_ne = 0;
        if (predict_step(&predictor, current_tick, &surprise_ne)) {
            neuromod_inject_ne(&neuromod, surprise_ne);
            softbody_jiggle(&sb, 0.22f);
            workspace_submit_candidate(&workspace, STIMULUS_SURPRISE_MISMATCH, fix_from_float(1.8f), current_tick);
        }

        /* ── Teclado ── */
        char key;
        while ((key = window_read_char(win)) != 0) {
            sensors_register_key(sensors, key, sim_t);
            behavior_dmn_signal_activity(&dmn, t_now);
            sense_fatigue_register_key(&sense_fatigue, key, sim_t);
            event_ring_push(&event_ring, EV_KEYSTROKE, current_tick, sim_t, 1.0f, "key");
            daily_summary_record_key(&daily_summary);
            sense_keystroke_press(&sense_keystroke, key, sim_t);

            if (sense_keystroke.owner_learned) {
                float sim_owner = sense_keystroke.last_similarity;
                if (sim_owner > 0.85f) {
                    neuromod_inject_ser(&neuromod, fix_from_float(0.005f));
                    drives.affection = (drives.affection < 0.99) ? (drives.affection + 0.002) : 1.0;
                } else if (sim_owner < 0.40f) {
                    neuromod_inject_ne(&neuromod, fix_from_float(0.01f));
                }
            }

            switch (key) {
            case KEY_SPACE:
                window_apply_impulse(win, 0.0, -680.0);
                softbody_stretch(&sb, 0.42f);
                break;
            case KEY_UP:
                window_apply_impulse(win, 0.0, -450.0);
                softbody_stretch(&sb, 0.30f);
                break;
            case KEY_DOWN:
                window_apply_impulse(win, 0.0, 360.0);
                softbody_squish(&sb, 0.30f);
                break;
            case KEY_LEFT:
                window_apply_impulse(win, -420.0, -120.0);
                softbody_jiggle(&sb, 0.28f);
                break;
            case KEY_RIGHT:
                window_apply_impulse(win, 420.0, -120.0);
                softbody_jiggle(&sb, 0.28f);
                break;
            case 'r': case 'R':
                brain_reset_habituation(brain);
                break;
            default:
                break;
            }
        }

        /* ── Alimenta dados de física da janela nos sensores ── */
        sensors_feed_physics(sensors, dt,
                             dragging, vx, vy, speed,
                             clicked, bumped,
                             cursor_x, cursor_y, pet_x, pet_y);

        sensors_feed_drives(sensors, &drives, cond.reward_trace, cond.punishment_trace);

        /* ══════════════════════════════════════════════════
         * CAMADA 2: SNN LIF + CPG + Biologia
         * ══════════════════════════════════════════════════ */
        double inputs[PESTINHA_CHANNELS] = {0};
        SensorReading sr = {0};
        sensors_poll(sensors, sim_t, &sr, inputs);

        /* Aplica ganho sensorial modulado pela Noradrenalina */
        fix16_t sens_gain = neuromod_get_sensory_gain(&neuromod);
        float f_gain = fix_to_float(sens_gain);
        for (int i = 0; i < PESTINHA_CHANNELS; ++i) {
            inputs[i] *= (double)f_gain;
            if (inputs[i] > 1.0) inputs[i] = 1.0;
        }

        brain_stimulate(brain, inputs);
        brain_step(brain, dt * 1000.0);

        /* Passo do CPG (Respiração biológica e batimento) */
        fix16_t arousal_stress = sat_add16(neuromod.cort, neuromod.ne);
        cpg_step(&cpg, arousal_stress, (uint32_t)brain_active_spike_count(brain), PESTINHA_NEURONS);
        neuromod_step(&neuromod);
        workspace_step(&workspace, current_tick);

        /* ── Ontogenia: Passo de maturação a 1 Hz (a cada 60 frames) ── */
        if (current_tick % 60 == 0) {
            fix16_t pool_err[ONTO_POOLS] = {0};
            fix16_t pool_act[ONTO_POOLS] = {0};

            for (int p = 0; p < ONTO_POOLS; ++p) {
                double r = brain_pool_rate(brain, p);
                int act_scaled = (int)(r * (256.0 / 40.0));
                if (act_scaled > 256) act_scaled = 256;
                if (act_scaled < 0) act_scaled = 0;
                pool_act[p] = (fix16_t)act_scaled;

                if (surprise_ne > 0 && p < PESTINHA_CHANNELS) {
                    pool_err[p] = surprise_ne > 256 ? 256 : surprise_ne;
                }
            }

            onto_tick_1hz(&ontogeny, pool_err, pool_act, neuromod.ne);

            /* Curiosidade por Progresso de Compressão (Schmidhuber) */
            static int prev_mean_err = -1;
            int sum_err = 0, n_active_pools = 0;
            for (int p = 0; p < ONTO_POOLS; ++p) {
                if (ontogeny.pool[p].seen) {
                    sum_err += (int)ontogeny.pool[p].err_ema;
                    n_active_pools++;
                }
            }
            if (n_active_pools > 0) {
                int mean_err = sum_err / n_active_pools;
                if (prev_mean_err >= 0) {
                    int delta_err = mean_err - prev_mean_err;
                    /* Progresso de Compressão: erro diminuindo -> recompensa dopaminérgica */
                    if (delta_err < -2) {
                        neuromod_inject_da(&neuromod, fix_from_float(0.04f));
                        drives.curiosity = (drives.curiosity > 0.05) ? (drives.curiosity - 0.02) : 0.0;
                    }
                    /* Tédio sustentado com erro baixo: estimula busca por novidade/exploração */
                    else if (abs(delta_err) <= 2 && mean_err < 40) {
                        drives.curiosity = (drives.curiosity < 0.95) ? (drives.curiosity + 0.015) : 1.0;
                    }
                }
                prev_mean_err = mean_err;
            }

            /* Avaliação das Regras Customizadas (.caddinrc) */
            time_t raw_t = time(NULL);
            struct tm *lt = localtime(&raw_t);
            int cur_hour = lt ? lt->tm_hour : 12;

            RuleContext rule_ctx = {
                .neuromod = &neuromod,
                .sb = &sb,
                .voice = &voice,
                .drives = &drives,
                .now_s = sim_t
            };
            config_rules_evaluate(&config_rules, current_win_title, cur_hour, sim_t, handle_rule_action, &rule_ctx);

            /* Gravação periódica de memória episódica na SDM a cada 30 segundos */
            if (current_tick % 1800 == 0) {
                record_episode_sdm(&sdm_memory, current_win_title, cur_hour, emotion.valence, drives.stress, 0);
            }

            /* Sincroniza maturação ontogenética com voz e corpo mole */
            voice_synth_set_maturity_stage(&voice, ontogeny.stage);
            softbody_set_maturity_stage(&sb, ontogeny.stage);

            /* Verificação de Marcos de Tempo Juntos e Rituais Circadianos */
            MainRitualContext ritual_ctx = {
                .voice = &voice,
                .sb = &sb,
                .neuromod = &neuromod,
                .drives = &drives,
                .now_s = sim_t
            };
            int cur_day = lt ? lt->tm_yday : 0;
            rituals_check_milestones(&rituals, ontogeny.awake_s, on_milestone_reached, &ritual_ctx);
            rituals_check_circadian(&rituals, cur_hour, cur_day, on_circadian_greeting, &ritual_ctx);

            /* Avaliação do Modelo de Fadiga e Intervenção pelo Bandit UCB1 */
            daily_summary_tick_1hz(&daily_summary, cur_day, behavior.asleep, (float)emotion.valence);
            sense_fatigue_update(&sense_fatigue, 1.0, sim_t);
            static double last_intervention_t = -1000.0;
            if (sense_fatigue_is_exhausted(&sense_fatigue) && (sim_t - last_intervention_t > 300.0)) {
                BanditArm arm = bandit_select_action(&bandit, sim_t);
                last_intervention_t = sim_t;
                event_ring_push(&event_ring, EV_INTERVENTION, current_tick, sim_t, (float)arm, "exhaustion_intervention");
                daily_summary_record_intervention(&daily_summary);

                switch (arm) {
                case ARM_SILENT:
                    softbody_squish(&sb, 0.15f);
                    break;
                case ARM_SUGGEST_PAUSE:
                    voice_synth_trigger_ex(&voice, VOICE_PURR_DEEP, 0.45f, 0.7f);
                    softbody_squish(&sb, 0.35f);
                    break;
                case ARM_CHEER_PLAY:
                    voice_synth_trigger_ex(&voice, VOICE_CHIRP_HAPPY, 0.30f, 0.85f);
                    window_apply_impulse(win, 0.0, -280.0);
                    softbody_stretch(&sb, 0.25f);
                    break;
                case ARM_STRETCH_WATER:
                    voice_synth_trigger_ex(&voice, VOICE_TRILL, 0.35f, 0.75f);
                    softbody_stretch(&sb, 0.40f);
                    softbody_jiggle(&sb, 0.25f);
                    break;
                default:
                    break;
                }
            }
        }

        /* ══════════════════════════════════════════════════
         * CAMADA 3: Neuromodulação Endógena + STDP
         * ══════════════════════════════════════════════════ */
        double endogenous_reward = homeostasis_compute_reward(&drives) + fix_to_float(neuromod.da - neuromod.base_da);
        double reward_factor = endogenous_reward
                             + cond.reward_trace - cond.punishment_trace;
        if (reward_factor > 1.5) reward_factor = 1.5;
        if (reward_factor < -1.5) reward_factor = -1.5;

        /* Modulação do aprendizado STDP pela Ontogenia (janela crítica) */
        fix16_t onto_scale = onto_lr_scale(&ontogeny, 0);
        double onto_mod = (double)onto_scale / 256.0;
        brain_learn_stdp(brain, reward_factor * onto_mod, dt);

        if (inputs[22] > 0.3) {
            conditioning_reward(&cond, inputs[22] * 0.3 * dt);
        }
        if (inputs[23] > 0.3) {
            conditioning_punish(&cond, inputs[23] * 0.3 * dt);
        }
        conditioning_update(&cond, dt);

        /* ══════════════════════════════════════════════════
         * CAMADA 4: Motores Neurais → Comportamento + Softbody
         * ══════════════════════════════════════════════════ */
        homeostasis_update(&drives, &behavior, dt,
                           behavior.wandering ? 1 : 0,
                           emotion.arousal, inputs);

        behavior_update(&behavior, &drives, &cond, brain, sim_t, dt);
        behavior_update_emotion(&emotion, &behavior, brain, sim_t, dt);

        /* Consolidação durante o sono */
        if (behavior.asleep) {
            lexicon_consolidate_sleep(&lexicon);
            sleep_accum_s += dt;
            if (sleep_accum_s >= 60.0) {
                onto_sleep_consolidate(&ontogeny);
                imprint_sleep(&imprint);
                sdm_consolidate_sleep(&sdm_memory);
                sense_keystroke_consolidate_imprint(&sense_keystroke);
                sleep_accum_s = 0.0;
            }
        } else {
            sleep_accum_s = 0.0;
        }

        /* Efeitos soft-body e voz procedurais */
        double m_purr = brain_motor(brain, M_PURR_EFFECT);
        double m_tail = brain_motor(brain, M_TAIL_WAG);
        double m_nod  = brain_motor(brain, M_NOD_HEAD);
        double m_shake_h = brain_motor(brain, M_SHAKE_HEAD);
        double m_jump = brain_motor(brain, M_JUMP_ACTION);
        double m_dance = brain_motor(brain, M_DANCE_ACTION);
        double m_chirp = brain_motor(brain, M_CHIRP_SOUND);

        if (m_purr > 0.25) {
            softbody_purr(&sb, (float)(m_purr * 2.0));
        }
        if (m_tail > 0.30) {
            softbody_jiggle(&sb, (float)(m_tail * 0.15));
        }
        if (m_nod > 0.35) {
            softbody_squish(&sb, (float)(m_nod * 0.08));
        }
        if (m_shake_h > 0.35) {
            softbody_jiggle(&sb, (float)(m_shake_h * 0.12));
        }
        if (m_jump > 0.40 && !dragging && speed < 30.0) {
            window_apply_impulse(win, 0.0, -350.0 * m_jump);
            softbody_stretch(&sb, 0.15f);
        }
        if (m_dance > 0.35) {
            softbody_jiggle(&sb, (float)(m_dance * 0.10));
        }
        if (m_chirp > 0.35 && (t_now - voice_last_chirp > VOICE_COOLDOWN_CHIRP)) {
            voice_synth_trigger_ex(&voice, VOICE_CHIRP_HAPPY, 0.20f, 0.7f);
            voice_last_chirp = t_now;
        }

        /* ── Emoções Reativas ── */
        emot_update(&em, dt, dragging, speed, vx, clicked, bumped);
        emot_sync_brain(&em, brain);

        /* ── Vocalização Orgânica: Dispara sons somente por emoções genuínas ── */
        if (em.current == EMO_AFFECTION) {
            /* Carinho — varia conforme duração do afeto:
             *   < 1s  = surpresa gostosa de ser tocado  → chirp feliz
             *   1-4s  = carinho moderado                → ronron suave
             *   > 4s  = êxtase de afeto prolongado      → ronron profundo + trilo */
            if (em.state_timer < 1.0f) {
                if (t_now - voice_last_chirp > VOICE_COOLDOWN_CHIRP) {
                    voice_synth_trigger_ex(&voice, VOICE_CHIRP_HAPPY, 0.22f, 0.75f);
                    voice_last_chirp = t_now;
                }
            } else if (em.state_timer < 4.0f) {
                if (t_now - voice_last_purr > VOICE_COOLDOWN_PURR) {
                    voice_synth_trigger_ex(&voice, VOICE_PURR_CONTENT, 0.40f,
                                           (float)(em.state_timer / 4.0f));
                    voice_last_purr = t_now;
                }
            } else {
                if (t_now - voice_last_purr > VOICE_COOLDOWN_PURR) {
                    voice_synth_trigger_ex(&voice, VOICE_PURR_DEEP, 0.50f, 0.9f);
                    voice_last_purr = t_now;
                }
                /* Trilo de alegria esporádico no êxtase */
                if (t_now - voice_last_trill > VOICE_COOLDOWN_TRILL) {
                    voice_synth_trigger_ex(&voice, VOICE_TRILL, 0.42f, 0.8f);
                    voice_last_trill = t_now;
                }
            }
        } else if (em.current == EMO_CURIOUS) {
            /* Curiosidade: pio interrogativo ascendente */
            if (t_now - voice_last_chirp > VOICE_COOLDOWN_CHIRP) {
                voice_synth_trigger_ex(&voice, VOICE_CHIRP_CURIOUS, 0.18f, 0.65f);
                voice_last_chirp = t_now;
            }
        } else if (em.current == EMO_SURPRISED) {
            /* Susto repentino: squeak agudo instantâneo */
            if (t_now - voice_last_alarm > VOICE_COOLDOWN_ALARM) {
                voice_synth_trigger_ex(&voice, VOICE_SQUEAK_STARTLED, 0.10f, 0.85f);
                voice_last_alarm = t_now;
            }
        } else if (em.current == EMO_ANNOYED) {
            /* Irritação — escalada: curta → growl, longa → hiss */
            if (em.state_timer < 2.0f) {
                if (t_now - voice_last_growl > VOICE_COOLDOWN_GROWL) {
                    voice_synth_trigger_ex(&voice, VOICE_GROWL_ANNOYED, 0.28f,
                                           (float)(em.state_timer / 2.0f));
                    voice_last_growl = t_now;
                }
            } else {
                if (t_now - voice_last_alarm > VOICE_COOLDOWN_ALARM) {
                    voice_synth_trigger_ex(&voice, VOICE_HISS_ANGRY, 0.08f, 0.9f);
                    voice_last_alarm = t_now;
                }
            }
        } else if (em.current == EMO_DIZZY) {
            /* Tontura/desconforto: reclamação suave */
            if (t_now - voice_last_whine > VOICE_COOLDOWN_WHINE) {
                voice_synth_trigger_ex(&voice, VOICE_WHINE_PROTEST, 0.30f, 0.6f);
                voice_last_whine = t_now;
            }
        }

        /* ── Autonomia: Passeio guiado pela SNN ── */
        autonomy_update(&autonomy, dt, win, &sb, &em, brain);

        /* ── Acoplamento do CPG (Respiração Visível) ao Softbody ── */
        float breath_expansion = fix_to_float(cpg_get_breath_expansion(&cpg));
        float target_sx = 1.0f + breath_expansion * 0.35f;
        float target_shear = 0.0f;
        if (dragging) {
            target_shear = (float)(-vx * 0.00065);
            if (speed > 250.0) {
                target_sx = 0.88f;
            }
        }
        softbody_update(&sb, dt, target_sx, target_shear);

        /* ── Geração do buffer de áudio procedural ── */
        size_t voice_samples = voice_synth_render(&voice, voice_pcm, 512);
        /* Envia SOMENTE as amostras geradas (0 quando VOICE_IDLE = silêncio) */
        if (audio_out_is_open() && voice_samples > 0) {
            audio_out_write(voice_pcm, voice_samples);
        }

        /* ── DMN (Default Mode Network) — comportamentos ociosos espontâneos ── */
        {
            fix16_t arousal_dmn = sat_add16(neuromod.da, neuromod.ne);
            DmnState dmn_state = behavior_dmn_update(&dmn, t_now, arousal_dmn, 20.0);
            switch (dmn_state) {
            case DMN_YAWN:
                softbody_stretch(&sb, 0.04f);
                /* Bocejo: som sonolento e orgânico */
                if (t_now - voice_last_grunt > VOICE_COOLDOWN_GRUNT) {
                    voice_synth_trigger_ex(&voice, VOICE_GRUNT_IDLE, 0.45f, 0.4f);
                    voice_last_grunt = t_now;
                }
                break;
            case DMN_GROOMING:
                softbody_jiggle(&sb, 0.06f);
                break;
            case DMN_MICRO_CHIRP:
                /* Chirp autônomo de curiosidade/vivacidade */
                if (t_now - voice_last_trill > 10.0) {
                    voice_synth_trigger_ex(&voice, VOICE_TRILL, 0.42f, 0.5f);
                    voice_last_trill = t_now;
                }
                break;
            case DMN_LOOK_AROUND:
                softbody_jiggle(&sb, 0.03f);
                break;
            default:
                break;
            }
        }

        /* ── Bond — atualiza vínculo de longo prazo ── */
        behavior_bond_update(&bond, t_now, dt);

        /* Bond: saudade prolongada → pede atenção discretamente a cada ~30s */
        if (behavior_bond_is_longing(&bond) && (current_tick % 1800 == 0)) {
            /* Saudade prolongada: reclamação suave de quem está sendo ignorado */
            if (t_now - voice_last_whine > VOICE_COOLDOWN_WHINE) {
                voice_synth_trigger_ex(&voice, VOICE_WHINE_PROTEST, 0.35f, 0.55f);
                voice_last_whine = t_now;
            }
            softbody_jiggle(&sb, 0.05f);
        }

        /* ── Exploração territorial (a cada 60 frames) ── */
        if (current_tick % 60 == 0) {
            behavior_explore_update_home(&explore, &sense_heatmap);
            fix16_t curiosity = neuromod.da;
            behavior_explore_update(&explore, &sense_heatmap,
                                    pet_x, pet_y, curiosity, t_now);
        }

        /* ── Animação e Renderização ── */
        anim_sync_brain(&anim, brain, &drives, &emotion);
        anim_update(&anim, dt);

        face_draw_squishy(canvas, &anim.current, anim.bob_y, anim.blink_lid, &sb, &em);
        window_present(win, canvas);

        /* ── Transmissão de Telemetria via FIFO (10 Hz, 100% C Puro) ── */
        if (current_tick % 6 == 0) {
            CaddinTelemetryFrame tframe;
            memset(&tframe, 0, sizeof(tframe));
            tframe.magic = TELEMETRY_MAGIC;
            tframe.tick = current_tick;
            tframe.timestamp_s = sim_t;
            tframe.da = fix_to_float(neuromod.da);
            tframe.ne = fix_to_float(neuromod.ne);
            tframe.ser = fix_to_float(neuromod.ser);
            tframe.cort = fix_to_float(neuromod.cort);
            tframe.onto_stage = (uint8_t)ontogeny.stage;
            tframe.onto_brake_mean = (float)onto_mean_brake(&ontogeny) / 65535.0f;
            tframe.fatigue = sense_fatigue_get_level(&sense_fatigue);
            tframe.bandit_arm = (uint8_t)(bandit.last_arm_chosen >= 0 ? bandit.last_arm_chosen : 0);
            tframe.valence = (float)emotion.valence;
            tframe.arousal = (float)emotion.arousal;
            for (int p = 0; p < 120; ++p) {
                double r = brain_pool_rate(brain, p);
                tframe.pool_activity[p] = (uint8_t)(r > 1.0 ? 255 : (r < 0.0 ? 0 : (r * 255.0)));
            }
            telemetry_fifo_send(&telemetry_fifo, &tframe);
        }

        /* ── Auto-save periódico dos pesos sinápticos e ontogenia ── */
        if (sim_t - last_save_t > AUTOSAVE_INTERVAL) {
            brain_save(brain, BRAIN_FILE);
            onto_save_state(&ontogeny, &imprint, ONTO_FILE);
            sdm_save_state(&sdm_memory, SDM_FILE);
            bandit_save_state(&bandit, BANDIT_FILE);
            keys_save_state(&sense_keystroke, KEYS_FILE);
            last_save_t = sim_t;
        }

        double elapsed = now_seconds() - t_now;
        if (elapsed < TARGET_DT)
            sleep_ms((int)((TARGET_DT - elapsed) * 1000.0));
    }

    /* ── Salva pesos sinápticos e ontogenia ao encerrar ── */
    brain_save(brain, BRAIN_FILE);
    onto_save_state(&ontogeny, &imprint, ONTO_FILE);
    sdm_save_state(&sdm_memory, SDM_FILE);
    bandit_save_state(&bandit, BANDIT_FILE);
    keys_save_state(&sense_keystroke, KEYS_FILE);
    daily_summary_append_file(&daily_summary, DAILY_LOG);

    printf("[SNN] Memória sináptica salva: %s\n", BRAIN_FILE);
    printf("[ONTO] Estado ontogenético salvo: %s (Estágio: %d, Vigília: %us, Ciclos sono: %u)\n",
           ONTO_FILE, ontogeny.stage, ontogeny.awake_s, ontogeny.sleeps);
    printf("[SDM] Memória episódica salva: %s\n", SDM_FILE);
    printf("[BANDIT] Modelo de intervenção salvo: %s\n", BANDIT_FILE);
    printf("[KEYSTROKE] Biometria salva: %s\n", KEYS_FILE);

    /* Imprime o Resumo Diário na saída padrão */
    char daily_buf[1024];
    if (daily_summary_format(&daily_summary, daily_buf, sizeof(daily_buf)) > 0) {
        printf("\n%s\n", daily_buf);
    }

    telemetry_fifo_close(&telemetry_fifo);

    sensors_close(sensors);
    brain_destroy(brain);
    audio_out_close();
    sense_mic_destroy(&sense_mic);
    if (sense_luma) sense_luma_destroy(sense_luma);
    window_destroy(win);
    return 0;
}
