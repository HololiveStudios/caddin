#ifndef PET_CORE_TELEMETRY_FIFO_H
#define PET_CORE_TELEMETRY_FIFO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define TELEMETRY_FIFO_PATH "/tmp/caddin_telemetry.fifo"
#define TELEMETRY_MAGIC 0x43414454 /* 'CADT' */

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t tick;
    double   timestamp_s;
    float    da;
    float    ne;
    float    ser;
    float    cort;
    uint8_t  onto_stage;
    float    onto_brake_mean;
    float    fatigue;
    uint8_t  bandit_arm;
    float    valence;
    float    arousal;
    uint8_t  pool_activity[120];
} CaddinTelemetryFrame;

typedef struct {
    int  fifo_fd;
    bool is_open;
    double last_broadcast_s;
} TelemetryFifo;

void telemetry_fifo_init(TelemetryFifo *tf);
void telemetry_fifo_close(TelemetryFifo *tf);

/* Envia um frame via FIFO sem bloquear (ignora se nenhum leitor estiver escutando) */
bool telemetry_fifo_send(TelemetryFifo *tf, const CaddinTelemetryFrame *frame);

#endif /* PET_CORE_TELEMETRY_FIFO_H */
