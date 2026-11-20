#include "daily_summary.h"
#include <stdio.h>
#include <string.h>

void daily_summary_init(DailySummary *ds, int day_of_year) {
    if (!ds) return;
    memset(ds, 0, sizeof(*ds));
    ds->current_day = day_of_year;
    ds->avg_valence_ema = 0.5f;
}

void daily_summary_tick_1hz(DailySummary *ds, int cur_day_of_year, bool is_asleep, float current_valence) {
    if (!ds) return;

    /* Detecta virada de dia e reseta contadores se mudou */
    if (cur_day_of_year != ds->current_day && ds->current_day >= 0) {
        ds->current_day = cur_day_of_year;
        ds->active_seconds = 0;
        ds->typing_keys_count = 0;
        ds->touches_count = 0;
        ds->shakes_count = 0;
        ds->code_errors_count = 0;
        ds->code_success_count = 0;
        ds->fatigue_interventions_count = 0;
        ds->nap_cycles_count = 0;
    }

    if (!is_asleep) {
        ds->active_seconds++;
        /* EMA suave do humor do pet */
        ds->avg_valence_ema = ds->avg_valence_ema * 0.999f + current_valence * 0.001f;
    }
}

void daily_summary_record_key(DailySummary *ds) {
    if (ds) ds->typing_keys_count++;
}

void daily_summary_record_touch(DailySummary *ds) {
    if (ds) ds->touches_count++;
}

void daily_summary_record_shake(DailySummary *ds) {
    if (ds) ds->shakes_count++;
}

void daily_summary_record_code_error(DailySummary *ds) {
    if (ds) ds->code_errors_count++;
}

void daily_summary_record_code_success(DailySummary *ds) {
    if (ds) ds->code_success_count++;
}

void daily_summary_record_intervention(DailySummary *ds) {
    if (ds) ds->fatigue_interventions_count++;
}

size_t daily_summary_format(const DailySummary *ds, char *out_buf, size_t max_len) {
    if (!ds || !out_buf || max_len == 0) return 0;

    uint32_t hours = ds->active_seconds / 3600;
    uint32_t mins  = (ds->active_seconds % 3600) / 60;
    uint32_t secs  = ds->active_seconds % 60;

    int written = snprintf(out_buf, max_len,
        "═══════════════════════════════════════════════\n"
        "           🐾 RESUMO DO DIA - CADDIN           \n"
        "═══════════════════════════════════════════════\n"
        " Dia do Ano          : %d\n"
        " Tempo Juntos Ativo  : %02uh %02um %02us\n"
        " Teclas Digitadas    : %u\n"
        " Carinhos Recebidos  : %u\n"
        " Sacudidas Sofridas  : %u\n"
        " Builds Sucesso      : %u\n"
        " Erros de Código     : %u\n"
        " Sugestões de Pausa  : %u\n"
        " Humor Médio do Pet  : %.2f\n"
        "═══════════════════════════════════════════════\n",
        ds->current_day,
        hours, mins, secs,
        ds->typing_keys_count,
        ds->touches_count,
        ds->shakes_count,
        ds->code_success_count,
        ds->code_errors_count,
        ds->fatigue_interventions_count,
        (double)ds->avg_valence_ema
    );

    if (written < 0) return 0;
    return (size_t)written;
}

bool daily_summary_append_file(const DailySummary *ds, const char *filepath) {
    if (!ds || !filepath) return false;

    FILE *f = fopen(filepath, "a");
    if (!f) return false;

    char buf[1024];
    size_t len = daily_summary_format(ds, buf, sizeof(buf));
    if (len > 0) {
        fwrite(buf, 1, len, f);
    }
    fclose(f);
    return true;
}
