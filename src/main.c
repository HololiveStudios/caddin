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
#include "sense/sense_mouse.h"
#include "sense/sense_scroll.h"
#include "sense/sense_windows.h"
#include "body/palette.h"
#include "body/voice_synth.h"
#include "mind/workspace.h"
#include "lang/lexicon.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#if defined(__linux__) && defined(__GLIBC__)
#  include <malloc.h>
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

/* Intervalo de auto-save (em segundos) */
#define AUTOSAVE_INTERVAL 60.0

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

int main(void) {
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

    /* ── novos subsistemas: biologia, percepção e mente ── */
    SenseMouse              sense_mouse;
    SenseScroll             sense_scroll;
    SenseWindows            sense_windows;
    Neuromodulators         neuromod;
    CentralPatternGenerator cpg;
    TemporalPredictor       predictor;
    GlobalWorkspace         workspace;
    Lexicon                 lexicon;
    VoiceSynth              voice;

    sense_mouse_init(&sense_mouse);
    sense_scroll_init(&sense_scroll);
    sense_windows_init(&sense_windows);
    neuromod_init(&neuromod);
    cpg_init(&cpg);
    predict_init(&predictor);
    workspace_init(&workspace);
    lexicon_init(&lexicon);
    voice_synth_init(&voice, 16000);

    /* ── animação, soft-body e emoções reativas ── */
    AnimState anim;
    anim_init(&anim);

    SoftBody sb;
    softbody_init(&sb);

    EmotionMachine em;
    emot_init(&em);

    AutonomyState autonomy;
    autonomy_init(&autonomy);

    static Canvas canvas;
    int16_t voice_pcm[512];

    const double TARGET_DT = 1.0 / 60.0;
    double t_last = now_seconds();
    double sim_t  = 0.0;
    double last_save_t = 0.0;
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
        {
            extern bool x11_get_screen_cursor(int *out_x, int *out_y);
            x11_get_screen_cursor(&cursor_x, &cursor_y);
        }

        /* ══════════════════════════════════════════════════
         * CAMADA 1: Percepção SO Avançada & Sensores
         * ══════════════════════════════════════════════════ */
        sense_mouse_update(&sense_mouse, cursor_x, cursor_y, pet_x, pet_y, 40);
        sense_scroll_step(&sense_scroll);

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
        }

        /* Reflexo Nociceptivo Espinhal (Sacudida violenta) */
        fix16_t shake_intensity = sense_mouse_get_shake_intensity(&sense_mouse);
        fix16_t motor_jolt = 0;
        if (neuromod_check_nociceptive_reflex(&neuromod, shake_intensity, &motor_jolt)) {
            softbody_squish(&sb, 0.45f);
            voice_synth_trigger(&voice, VOICE_ALARM, 0.20f);
            workspace_submit_candidate(&workspace, STIMULUS_VIOLENT_SHAKE, fix_from_float(2.5f), current_tick);
            lexicon_observe_word(&lexicon, "susto", fix_from_float(-0.9f), fix_from_float(0.9f), 2);
            drives.stress = (drives.stress < 0.95) ? (drives.stress + 0.15) : 1.0;
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
        }

        /* Predição Temporal: Mismatch Spikes (Surpresa) */
        fix16_t surprise_ne = 0;
        if (predict_step(&predictor, current_tick, &surprise_ne)) {
            neuromod_inject_ne(&neuromod, surprise_ne);
            voice_synth_trigger(&voice, VOICE_CHIRP, 0.15f);
            softbody_jiggle(&sb, 0.22f);
            workspace_submit_candidate(&workspace, STIMULUS_SURPRISE_MISMATCH, fix_from_float(1.8f), current_tick);
        }

        /* ── Teclado ── */
        char key;
        while ((key = window_read_char(win)) != 0) {
            sensors_register_key(sensors, key, sim_t);

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

        /* ══════════════════════════════════════════════════
         * CAMADA 3: Neuromodulação Endógena + STDP
         * ══════════════════════════════════════════════════ */
        double endogenous_reward = homeostasis_compute_reward(&drives) + fix_to_float(neuromod.da - neuromod.base_da);
        double reward_factor = endogenous_reward
                             + cond.reward_trace - cond.punishment_trace;
        if (reward_factor > 1.5) reward_factor = 1.5;
        if (reward_factor < -1.5) reward_factor = -1.5;

        brain_learn_stdp(brain, reward_factor, dt);

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
            voice_synth_trigger(&voice, VOICE_PURR, 0.25f);
        }
        if (m_chirp > 0.35) {
            voice_synth_trigger(&voice, VOICE_CHIRP, 0.15f);
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

        /* ── Emoções Reativas ── */
        emot_update(&em, dt, dragging, speed, vx, clicked, bumped);
        emot_sync_brain(&em, brain);

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
        voice_synth_render(&voice, voice_pcm, 512);

        /* ── Animação e Renderização ── */
        anim_sync_brain(&anim, brain, &drives, &emotion);
        anim_update(&anim, dt);

        face_draw_squishy(canvas, &anim.current, anim.bob_y, anim.blink_lid, &sb, &em);
        window_present(win, canvas);

        /* ── Auto-save periódico dos pesos sinápticos ── */
        if (sim_t - last_save_t > AUTOSAVE_INTERVAL) {
            brain_save(brain, BRAIN_FILE);
            last_save_t = sim_t;
        }

        double elapsed = now_seconds() - t_now;
        if (elapsed < TARGET_DT)
            sleep_ms((int)((TARGET_DT - elapsed) * 1000.0));
    }

    /* ── Salva pesos sinápticos ao encerrar ── */
    brain_save(brain, BRAIN_FILE);
    printf("[SNN] Memória sináptica salva: %s\n", BRAIN_FILE);

    sensors_close(sensors);
    brain_destroy(brain);
    window_destroy(win);
    return 0;
}
