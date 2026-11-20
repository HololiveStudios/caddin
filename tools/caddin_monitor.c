#include "telemetry_fifo.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#if defined(__linux__) || defined(__unix__)
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

static void draw_bar(const char *label, float val, float max_val, const char *color_ansi) {
    if (val < 0.0f) val = 0.0f;
    if (val > max_val) val = max_val;
    int total_chars = 24;
    int filled = (int)((val / max_val) * (float)total_chars);
    if (filled > total_chars) filled = total_chars;

    printf("%-6s %s[", label, color_ansi);
    for (int i = 0; i < total_chars; ++i) {
        if (i < filled) printf("█");
        else printf("░");
    }
    printf("\033[0m] %5.2f\n", (double)val);
}

int main(void) {
    printf("\033[2J\033[H"); /* Limpa terminal */
    printf("\033[1;36m══════════════════════════════════════════════════════════════\033[0m\n");
    printf("\033[1;32m       🐾 CADDIN NEURAL TELEMETRY MONITOR (100%% C PURE)       \033[0m\n");
    printf("\033[1;36m══════════════════════════════════════════════════════════════\033[0m\n");
    printf("Aguardando transmissão do mascote Caddin em %s...\n", TELEMETRY_FIFO_PATH);

    mkfifo(TELEMETRY_FIFO_PATH, 0666);
    int fd = open(TELEMETRY_FIFO_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Erro ao abrir FIFO");
        return 1;
    }

    CaddinTelemetryFrame frame;
    const char *stage_names[] = {"FILHOTE", "JUVENIL", "ADULTO", "VETERANO"};
    const char *bandit_arms[] = {"SILENT", "PAUSA", "ALEGRE", "ALONGAR"};

    while (1) {
        ssize_t bytes = read(fd, &frame, sizeof(frame));
        if (bytes <= 0) {
            /* Reconecta se o processo principal reiniciar */
            close(fd);
            usleep(200000);
            fd = open(TELEMETRY_FIFO_PATH, O_RDONLY);
            if (fd < 0) break;
            continue;
        }

        if (bytes == sizeof(frame) && frame.magic == TELEMETRY_MAGIC) {
            printf("\033[H"); /* Retorna o cursor para o topo */
            printf("\033[1;36m══════════════════════════════════════════════════════════════\033[0m\n");
            printf("\033[1;32m       🐾 CADDIN NEURAL TELEMETRY MONITOR (100%% C PURE)       \033[0m\n");
            printf("\033[1;36m══════════════════════════════════════════════════════════════\033[0m\n");
            printf(" Tick: %-8u | Tempo: %7.1fs | Estágio: \033[1;33m%-8s\033[0m | Freio: %4.1f%%\n",
                   frame.tick, frame.timestamp_s,
                   (frame.onto_stage < 4 ? stage_names[frame.onto_stage] : "UNKNOWN"),
                   (double)(frame.onto_brake_mean * 100.0f));
            printf("──────────────────────────────────────────────────────────────\n");
            printf("\033[1;37m[DINÂMICA DE NEUROTRANSMISSORES]\033[0m\n");
            draw_bar("DA",   frame.da,   1.5f, "\033[1;32m");
            draw_bar("NE",   frame.ne,   1.5f, "\033[1;31m");
            draw_bar("SER",  frame.ser,  1.5f, "\033[1;34m");
            draw_bar("CORT", frame.cort, 1.5f, "\033[1;35m");

            printf("──────────────────────────────────────────────────────────────\n");
            printf("\033[1;37m[ERGONOMIA & COMPORTAMENTO]\033[0m\n");
            draw_bar("FADIGA", frame.fatigue, 1.0f, "\033[1;33m");
            printf(" Bandit Intervenção : \033[1;36m%s\033[0m\n",
                   (frame.bandit_arm < 4 ? bandit_arms[frame.bandit_arm] : "NONE"));
            printf(" Humor / Valência   : %5.2f | Arousal: %5.2f\n",
                   (double)frame.valence, (double)frame.arousal);

            printf("──────────────────────────────────────────────────────────────\n");
            printf("\033[1;37m[ATIVIDADE DOS 120 POOLS NEURAIS SNN]\033[0m\n");
            for (int i = 0; i < 120; ++i) {
                if (i > 0 && i % 30 == 0) printf("\n");
                uint8_t act = frame.pool_activity[i];
                if (act > 180)      printf("\033[1;31m█\033[0m");
                else if (act > 100) printf("\033[1;33m▓\033[0m");
                else if (act > 30)  printf("\033[1;32m▒\033[0m");
                else                printf("\033[0;37m░\033[0m");
            }
            printf("\n\033[1;36m══════════════════════════════════════════════════════════════\033[0m\n");
            fflush(stdout);
        }
    }

    close(fd);
    return 0;
}
#else
int main(void) {
    printf("Telemetria via FIFO está disponível nativamente em sistemas Linux/POSIX.\n");
    return 0;
}
#endif
