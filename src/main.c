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

    /* ── Luma precisa de Display*, criado dinamicamente ── */
    SenseLuma *sense_luma = NULL;
#if defined(__linux__) && !defined(_WIN32)
    {
        /* Inclui X11 apenas no bloco guardado — evita exposição do tipo Display
         * ao compilador do main.c que não inclui Xlib diretamente */
        void *dpy2 = NULL;
        unsigned long root2 = 0;
        {
            /* x11_get_display retorna struct _XDisplay* (opaco fora de Xlib),
             * mas sense_luma_create o aceita como void* */
            extern void *x11_get_display(void);
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

    static Canvas canvas;
    int16_t voice_pcm[512];

    /* Cooldowns de voz — impedem re-trigger enquanto o som anterior ainda toca.
     * Cada slot guarda o timestamp do último trigger para aquele tipo de voz. */
    double voice_last_purr  = -99.0;
    double voice_last_chirp = -99.0;
    double voice_last_alarm = -99.0;
    /* Duração mínima entre triggers (segundos) */
    #define VOICE_COOLDOWN_PURR   3.0   /* ronron: longo, não repete antes de 3s */
    #define VOICE_COOLDOWN_CHIRP  1.5   /* pio: re-ativa no máximo a cada 1.5s */
    #define VOICE_COOLDOWN_ALARM  2.0   /* susto: cooldown de 2s */

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

        /* Fix 2: centro real do pet (janela 192×144, centro = +96, +72) */
        int pet_cx = pet_x + WINDOW_W / 2;
        int pet_cy = pet_y + WINDOW_H / 2;
        sense_mouse_update(&sense_mouse, cursor_x, cursor_y, pet_cx, pet_cy, 48);

        /* Fix 3: scroll wheel */
        {
            int wheel = window_get_wheel(win);
            if (wheel != 0) {
                sense_scroll_register(&sense_scroll, wheel);
            }
        }
        sense_scroll_step(&sense_scroll);

        /* Fix 4: janela ativa — detecção de zona de perigo e troca de foco */
        {
            int aw_x, aw_y, aw_ww, aw_hh;
            uint64_t aw_id = 0;
            if (window_get_active_window_rect(win, &aw_x, &aw_y, &aw_ww, &aw_hh, &aw_id)) {
                sense_windows_update(&sense_windows, aw_id,
                                     aw_x, aw_y, aw_ww, aw_hh);
                if (sense_windows_did_switch_focus(&sense_windows)) {
                    neuromod_inject_ne(&neuromod, fix_from_float(0.05f));
                    softbody_jiggle(&sb, 0.18f);
                }
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
        }

        /* Reflexo Nociceptivo Espinhal (Sacudida violenta) */
        fix16_t shake_intensity = sense_mouse_get_shake_intensity(&sense_mouse);
        fix16_t motor_jolt = 0;
        if (neuromod_check_nociceptive_reflex(&neuromod, shake_intensity, &motor_jolt)) {
            softbody_squish(&sb, 0.45f);
            if (t_now - voice_last_alarm > VOICE_COOLDOWN_ALARM) {
                voice_synth_trigger(&voice, VOICE_ALARM, 0.20f);
                voice_last_alarm = t_now;
            }
            workspace_submit_candidate(&workspace, STIMULUS_VIOLENT_SHAKE, fix_from_float(2.5f), current_tick);
            lexicon_observe_word(&lexicon, "susto", fix_from_float(-0.9f), fix_from_float(0.9f), 2);
            drives.stress = (drives.stress < 0.95) ? (drives.stress + 0.15) : 1.0;
            behavior_bond_on_shake(&bond);
            behavior_dmn_signal_activity(&dmn, t_now);
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

        /* ── Vocalização Orgânica: Dispara sons somente por emoções genuínas ── */
        if (em.current == EMO_AFFECTION) {
            /* Carinho suave / amor: ronrom relaxante */
            if (t_now - voice_last_purr > VOICE_COOLDOWN_PURR) {
                voice_synth_trigger(&voice, VOICE_PURR, 0.35f);
                voice_last_purr = t_now;
            }
        } else if (em.current == EMO_CURIOUS) {
            /* Curiosidade / toque pontual: pio fofo */
            if (t_now - voice_last_chirp > VOICE_COOLDOWN_CHIRP) {
                voice_synth_trigger(&voice, VOICE_CHIRP, 0.12f);
                voice_last_chirp = t_now;
            }
        } else if (em.current == EMO_SURPRISED) {
            /* Susto repentino ou arremesso rápido */
            if (t_now - voice_last_chirp > 1.2) {
                voice_synth_trigger(&voice, VOICE_CHIRP, 0.15f);
                voice_last_chirp = t_now;
            }
        } else if (em.current == EMO_ANNOYED || em.current == EMO_DIZZY) {
            /* Irritação por sacudida contínua na janela: reclamação expressiva */
            if (t_now - voice_last_alarm > VOICE_COOLDOWN_ALARM) {
                voice_synth_trigger(&voice, VOICE_ALARM, 0.22f);
                voice_last_alarm = t_now;
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
                break;
            case DMN_GROOMING:
                softbody_jiggle(&sb, 0.06f);
                break;
            case DMN_MICRO_CHIRP:
                if (t_now - voice_last_chirp > 10.0) {
                    voice_synth_trigger(&voice, VOICE_CHIRP, 0.08f);
                    voice_last_chirp = t_now;
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
            if (t_now - voice_last_chirp > 15.0) {
                voice_synth_trigger(&voice, VOICE_CHIRP, 0.10f);
                voice_last_chirp = t_now;
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
    audio_out_close();
    if (sense_luma) sense_luma_destroy(sense_luma);
    window_destroy(win);
    return 0;
}
