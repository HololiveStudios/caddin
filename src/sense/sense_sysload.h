/* sense_sysload.h — percepção de carga do sistema (CPU, temperatura)
 * Lê /proc/stat (Linux) ou GetSystemTimes() (Windows).
 * Sem alocações dinâmicas. Período de atualização configurável.
 */
#ifndef SENSE_SYSLOAD_H
#define SENSE_SYSLOAD_H

#include <stdbool.h>
#include "../core/fixed.h"

typedef struct {
    fix16_t cpu_load;       /* 0..1 (Q8.8) — uso de CPU no último segundo */
    fix16_t cpu_temp;       /* 0..1 (Q8.8) — temperatura normalizada (0=20°C, 1=100°C) */
    fix16_t mem_pressure;   /* 0..1 — pressão de memória (RAM livre/total) */
    double  last_update;    /* timestamp da última leitura */

    /* estado interno Linux */
    long long prev_idle;
    long long prev_total;
} SenseSysload;

void     sense_sysload_init(SenseSysload *s);
/* Atualiza leituras (máximo 1x/seg — retorna false se muito cedo) */
bool     sense_sysload_update(SenseSysload *s, double now_sec);

fix16_t  sense_sysload_get_cpu(const SenseSysload *s);
fix16_t  sense_sysload_get_temp(const SenseSysload *s);
fix16_t  sense_sysload_get_mem(const SenseSysload *s);

#endif /* SENSE_SYSLOAD_H */
