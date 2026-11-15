#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "terminal_ui.h"
#include "neuro.h"
#include "homeostasis.h"
#include "behavior.h"
#include "conditioning.h"
#include "memory.h"
#include "sensors.h"
#include "runtime_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
static void sleep_ms(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}
#define SLEEP_MS(ms) sleep_ms(ms)
#else
#include <windows.h>
#define SLEEP_MS(ms) Sleep(ms)
#endif

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    TerminalState term;
    terminal_ui_init(&term);

    Brain *brain = brain_create(42, "jorge");
    if (!brain) {
        fprintf(stderr, "Erro ao inicializar a Rede Neural de 1024 neurônios.\n");
        terminal_ui_cleanup(&term);
        return 1;
    }

    Drives drives;
    Emotion emotion;
    FaceState face;
    Behavior behavior;
    ConditioningState conditioning;
    EpisodeMemory memory;
    Sensors *sensors = sensors_open();

    homeostasis_init(&drives);
    behavior_init(&behavior, &emotion, &face);
    conditioning_init(&conditioning);
    memory_init(&memory);

    bool running = true;
    double now_t = 0.0;
    const double dt = 1.0 / 30.0; /* 30 FPS no terminal */
    double inputs[PESTINHA_CHANNELS] = {0};

    clock_t last_clock = clock();

    while (running) {
        now_t += dt;

        /* A. Leitura de teclas não-bloqueante no terminal */
        char key = terminal_ui_read_key();
        if (key != 0) {
            sensors_register_key(sensors, key, now_t);
            face.last_key_pressed = key;

            if (key == 'c' || key == 'C') {
                homeostasis_event(&drives, "caress");
                conditioning_reward(&conditioning, 0.3);
                memory_record(&memory, "caress", now_t);
                brain_learn_stdp(brain, 0.8, dt);
                face.action_msg_timer = 2.5;
                snprintf(face.action_msg, sizeof(face.action_msg), "Voce fez carinho no Pestinha! (+Afeto, -Estresse)");
            } else if (key == 'p' || key == 'P') {
                homeostasis_event(&drives, "poke");
                conditioning_punish(&conditioning, 0.4);
                memory_record(&memory, "poke", now_t);
                brain_learn_stdp(brain, -0.6, dt);
                face.action_msg_timer = 2.5;
                snprintf(face.action_msg, sizeof(face.action_msg), "Voce cutucou o Pestinha! (+Estresse, -Afeto)");
            } else if (key == 'f' || key == 'F') {
                homeostasis_event(&drives, "feed");
                conditioning_reward(&conditioning, 0.4);
                memory_record(&memory, "feed", now_t);
                face.action_msg_timer = 2.5;
                snprintf(face.action_msg, sizeof(face.action_msg), "Voce alimentou o Pestinha! (+Saciedade, +Energia)");
            } else if (key == 't' || key == 'T') {
                homeostasis_event(&drives, "talk");
                face.action_msg_timer = 2.5;
                snprintf(face.action_msg, sizeof(face.action_msg), "Voce conversou com o Pestinha! (+Curiosidade)");
            } else if (key == 'r' || key == 'R') {
                brain_reset_habituation(brain);
                face.action_msg_timer = 2.5;
                snprintf(face.action_msg, sizeof(face.action_msg), "Habituacao dos neuronios sensoriais resetada!");
            } else if (key == 'q' || key == 'Q') {
                running = false;
            } else {
                /* Qualquer outra tecla digitada pelo usuário */
                face.action_msg_timer = 1.5;
                snprintf(face.action_msg, sizeof(face.action_msg), "Tecla '%c' capturada! (Estilo sensorial enviado)", key);
            }
        }

        /* B. Atualização dos Sensores */
        SensorReading sensor_data = {0};
        if (sensors) {
            sensors_poll(sensors, now_t, &sensor_data, inputs);
        }

        /* C. Estimula e Avança a Rede Neural Pulsada (SNN - 1024 neurônios) */
        brain_stimulate(brain, inputs);
        brain_step(brain, dt * 1000.0);

        /* D. Aplica Plasticidade STDP Trifatorial contínua */
        brain_learn_stdp(brain, conditioning.reward_trace - conditioning.punishment_trace, dt);

        /* E. Atualiza Homeostase, Comportamento e Emoção */
        homeostasis_update(&drives, &behavior, dt, behavior.wandering ? 1 : 0, emotion.arousal, inputs);
        behavior_update(&behavior, &drives, &conditioning, brain, now_t, dt);
        behavior_update_emotion(&emotion, &behavior, &face, brain, now_t, dt);
        conditioning_update(&conditioning, dt);

        /* F. Renderiza o Terminal */
        clock_t cur_clock = clock();
        double elapsed_sec = (double)(cur_clock - last_clock) / CLOCKS_PER_SEC;
        double current_fps = (elapsed_sec > 0.0) ? (1.0 / elapsed_sec) : 30.0;
        last_clock = cur_clock;

        terminal_ui_render(brain, &drives, &emotion, &behavior, &face, &conditioning, current_fps, now_t);

        SLEEP_MS(33); /* ~30 FPS */
    }

    sensors_close(sensors);
    brain_destroy(brain);
    terminal_ui_cleanup(&term);

    printf("\n\nSessao encerrada. Ate logo!\n");
    return 0;
}
