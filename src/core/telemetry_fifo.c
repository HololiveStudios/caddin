#include "telemetry_fifo.h"
#include <string.h>

#if defined(__linux__) || defined(__unix__)
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

void telemetry_fifo_init(TelemetryFifo *tf) {
    if (!tf) return;
    tf->fifo_fd = -1;
    tf->is_open = false;
    tf->last_broadcast_s = 0.0;

    /* Garante existência da FIFO */
    mkfifo(TELEMETRY_FIFO_PATH, 0666);
}

void telemetry_fifo_close(TelemetryFifo *tf) {
    if (!tf) return;
    if (tf->fifo_fd >= 0) {
        close(tf->fifo_fd);
        tf->fifo_fd = -1;
    }
    tf->is_open = false;
}

bool telemetry_fifo_send(TelemetryFifo *tf, const CaddinTelemetryFrame *frame) {
    if (!tf || !frame) return false;

    if (tf->fifo_fd < 0) {
        /* Tenta abrir em modo não-bloqueante */
        tf->fifo_fd = open(TELEMETRY_FIFO_PATH, O_WRONLY | O_NONBLOCK);
        if (tf->fifo_fd < 0) {
            /* Sem leitor conectado (ENXIO é esperado em O_NONBLOCK sem leitor) */
            return false;
        }
        tf->is_open = true;
    }

    ssize_t written = write(tf->fifo_fd, frame, sizeof(*frame));
    if (written < 0) {
        if (errno == EPIPE || errno == ENXIO) {
            /* Leitor desconectou */
            close(tf->fifo_fd);
            tf->fifo_fd = -1;
            tf->is_open = false;
        }
        return false;
    }

    return (written == sizeof(*frame));
}

#else
/* Stub para sistemas sem FIFOs POSIX */
void telemetry_fifo_init(TelemetryFifo *tf) {
    if (tf) { tf->fifo_fd = -1; tf->is_open = false; tf->last_broadcast_s = 0.0; }
}
void telemetry_fifo_close(TelemetryFifo *tf) { (void)tf; }
bool telemetry_fifo_send(TelemetryFifo *tf, const CaddinTelemetryFrame *frame) {
    (void)tf; (void)frame;
    return false;
}
#endif
