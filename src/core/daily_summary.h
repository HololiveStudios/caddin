#ifndef PET_CORE_DAILY_SUMMARY_H
#define PET_CORE_DAILY_SUMMARY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int      current_day;
    uint32_t active_seconds;
    uint32_t typing_keys_count;
    uint32_t touches_count;
    uint32_t shakes_count;
    uint32_t code_errors_count;
    uint32_t code_success_count;
    uint32_t fatigue_interventions_count;
    uint32_t nap_cycles_count;
    float    avg_valence_ema;
} DailySummary;

void daily_summary_init(DailySummary *ds, int day_of_year);

/* Atualização contínua a 1 Hz */
void daily_summary_tick_1hz(DailySummary *ds, int cur_day_of_year, bool is_asleep, float current_valence);

/* Registro de eventos discretos */
void daily_summary_record_key(DailySummary *ds);
void daily_summary_record_touch(DailySummary *ds);
void daily_summary_record_shake(DailySummary *ds);
void daily_summary_record_code_error(DailySummary *ds);
void daily_summary_record_code_success(DailySummary *ds);
void daily_summary_record_intervention(DailySummary *ds);

/* Formata resumo em buffer de texto (sem malloc) */
size_t daily_summary_format(const DailySummary *ds, char *out_buf, size_t max_len);

/* Grava o resumo diário no arquivo log (ex: "caddin_daily.log") */
bool daily_summary_append_file(const DailySummary *ds, const char *filepath);

#endif /* PET_CORE_DAILY_SUMMARY_H */
