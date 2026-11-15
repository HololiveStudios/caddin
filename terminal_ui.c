#include "terminal_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(__unix__) || defined(__APPLE__)
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

static struct termios orig_termios;
static bool termios_raw_active = false;

static void disable_raw_mode(void) {
    if (termios_raw_active) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        termios_raw_active = false;
        printf("\033[?25h\033[0m\n"); /* Mostra o cursor e reseta cores */
        fflush(stdout);
    }
}

static void enable_raw_mode(void) {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == 0) {
        struct termios raw = orig_termios;
        /* Não removemos OPOST para manter a formatação de \n perfeita */
        raw.c_lflag &= ~(ECHO | ICANON);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        termios_raw_active = true;
        atexit(disable_raw_mode);
        printf("\033[?25l"); /* Esconde o cursor */
        fflush(stdout);
    }
}

char terminal_ui_read_key(void) {
    fd_set fds;
    struct timeval tv = {0, 0};
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    int ret = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    if (ret > 0 && FD_ISSET(STDIN_FILENO, &fds)) {
        char c = 0;
        if (read(STDIN_FILENO, &c, 1) == 1) {
            return c;
        }
    }
    return 0;
}
#else
#include <conio.h>
static void disable_raw_mode(void) {}
static void enable_raw_mode(void) {}
char terminal_ui_read_key(void) {
    if (_kbhit()) {
        return (char)_getch();
    }
    return 0;
}
#endif

void terminal_ui_init(TerminalState *term) {
    if (term) {
        term->ansi_supported = true;
        term->running = true;
    }
    enable_raw_mode();
}

void terminal_ui_cleanup(TerminalState *term) {
    (void)term;
    disable_raw_mode();
}

static void draw_bar(char *buf, size_t size, double value, int width) {
    if (value < 0.0) value = 0.0;
    if (value > 1.0) value = 1.0;
    int filled = (int)round(value * width);
    size_t pos = 0;
    buf[pos++] = '[';
    for (int i = 0; i < width; ++i) {
        if (i < filled) {
            pos += snprintf(buf + pos, size - pos, "#");
        } else {
            pos += snprintf(buf + pos, size - pos, "-");
        }
    }
    buf[pos++] = ']';
    buf[pos] = '\0';
}

void terminal_ui_render(const Brain *brain, const Drives *drives, const Emotion *emotion,
                        const Behavior *behavior, const FaceState *face,
                        const ConditioningState *cond, double fps, double now_t) {
    (void)cond; (void)behavior;
    char bar_energy[64], bar_stress[64], bar_curiosity[64], bar_satiety[64], bar_affection[64];

    draw_bar(bar_energy, sizeof(bar_energy), drives ? drives->energy : 0.0, 12);
    draw_bar(bar_stress, sizeof(bar_stress), drives ? drives->stress : 0.0, 12);
    draw_bar(bar_curiosity, sizeof(bar_curiosity), drives ? drives->curiosity : 0.0, 12);
    draw_bar(bar_satiety, sizeof(bar_satiety), drives ? drives->satiety : 0.0, 12);
    draw_bar(bar_affection, sizeof(bar_affection), drives ? drives->affection : 0.0, 12);

    int active_spikes = brain_active_spike_count(brain);
    double plastic_delta = brain_total_plastic_weight(brain);

    /* Mensagem a exibir: Mensagem de ação com prioridade quando o timer estiver ativo */
    const char *display_status = (face && face->action_msg_timer > 0.0 && face->action_msg[0] != '\0')
        ? face->action_msg : (face ? face->status_msg : "Em observacao...");

    /* Volta para o topo da tela no terminal (ANSI home + clear) */
    printf("\033[H\033[J");

    printf("\033[1;36m========================================================================\033[0m\n");
    printf("\033[1;33m       PESTINHA NEURAL - SIMULADOR SNN 1024 NEURONIOS (C99 TERMINAL)    \033[0m\n");
    printf("\033[1;36m========================================================================\033[0m\n\n");

    /* Painel Principal: Rosto ASCII + Telemetria */
    printf("\033[1;32m  ┌─────────────────────────────────┐\033[0m    \033[1;35m[ TELEMETRIA E VITAIS ]\033[0m\n");
    printf("\033[1;32m  │                                 │\033[0m    Energia   : %s %3.0f%%\n", bar_energy, (drives ? drives->energy : 0.0) * 100.0);
    printf("\033[1;32m  │     \033[1;37m%-19s\033[1;32m     │\033[0m    Estresse  : %s %3.0f%%\n", face ? face->face_str : "( o _ o )", bar_stress, (drives ? drives->stress : 0.0) * 100.0);
    printf("\033[1;32m  │                                 │\033[0m    Curiosid. : %s %3.0f%%\n", bar_curiosity, (drives ? drives->curiosity : 0.0) * 100.0);
    printf("\033[1;32m  └─────────────────────────────────┘\033[0m    Saciedade : %s %3.0f%%\n", bar_satiety, (drives ? drives->satiety : 0.0) * 100.0);
    printf("                                         Afeto     : %s %3.0f%%\n\n", bar_affection, (drives ? drives->affection : 0.0) * 100.0);

    /* Mensagem de Estado e Interação em Destaque */
    if (face && face->action_msg_timer > 0.0) {
        printf("\033[1;32m  [INTERACAO ATIVA]:\033[0m \033[1;37m%-48s\033[0m\n", display_status);
    } else {
        printf("\033[1;33m  [Status]:\033[0m %-48s\n", display_status);
    }

    printf("\033[1;34m  [Humor] :\033[0m Valência: %+.2f | Excitação: %.2f | FPS: %.1f | Tempo: %.1fs\n\n",
           emotion ? emotion->valence : 0.0, emotion ? emotion->arousal : 0.0, fps, now_t);

    /* Painel da Rede Neural SNN (1024 Neurônios + STDP) */
    printf("\033[1;36m------------------------------------------------------------------------\033[0m\n");
    printf("\033[1;37m  [MOTOR NEURAL SNN (1024 NEURONIOS - MODELO LIF)]\033[0m\n");
    printf("  - Neurônios Ativos no Frame : \033[1;31m%-4d\033[0m / 1024 neurônios\n", active_spikes);
    printf("  - Variação Sináptica STDP   : \033[1;33m%.4f\033[0m (Plasticidade Ativa)\n", plastic_delta);
    printf("  - Canais Sensoriais Ativos  : \033[1;32m64/64\033[0m | Interneurônios: \033[1;32m576\033[0m | Motores: \033[1;32m192\033[0m\n");
    if (face && face->last_key_pressed != 0) {
        printf("  - Última Tecla Pressionada  : \033[1;33m'%c'\033[0m\n", face->last_key_pressed);
    }
    printf("\033[1;36m------------------------------------------------------------------------\033[0m\n\n");

    /* Barra de Comandos e Controles */
    printf("\033[1;37m  [CONTROLES INTERATIVOS DO TERMINAL]:\033[0m\n");
    printf("  \033[1;32m[C]\033[0m Carinho   | \033[1;31m[P]\033[0m Cutucar | \033[1;33m[F]\033[0m Alimentar | \033[1;34m[T]\033[0m Conversar | \033[1;35m[R]\033[0m Reset Habituação | \033[1;37m[Q]\033[0m Sair\n");
    fflush(stdout);
}
