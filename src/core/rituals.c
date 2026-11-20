#include "rituals.h"
#include <string.h>

void rituals_init(RitualState *rs) {
    if (!rs) return;
    memset(rs, 0, sizeof(*rs));
    rs->last_checked_day = -1;
}

void rituals_check_milestones(RitualState *rs,
                              uint32_t awake_seconds,
                              MilestoneCallback callback,
                              void *user_data) {
    if (!rs || !callback) return;

    static const uint32_t MILESTONES_HOURS[] = { 1, 5, 10, 25, 50, 100, 250, 500, 1000 };
    uint32_t current_hours = awake_seconds / 3600;

    for (size_t i = 0; i < sizeof(MILESTONES_HOURS)/sizeof(MILESTONES_HOURS[0]); ++i) {
        uint32_t m = MILESTONES_HOURS[i];
        if (current_hours >= m && rs->last_milestone_hours < m) {
            rs->last_milestone_hours = m;
            callback(m, user_data);
            break;
        }
    }
}

void rituals_check_circadian(RitualState *rs,
                             int current_hour,
                             int current_day,
                             CircadianCallback callback,
                             void *user_data) {
    if (!rs || !callback) return;

    /* Novo dia calendário: reseta saudações diárias */
    if (rs->last_checked_day != current_day) {
        rs->last_checked_day = current_day;
        rs->morning_greeting_done = false;
        rs->night_farewell_done = false;
    }

    /* Saudação Matinal: entre 6h e 11h */
    if (current_hour >= 6 && current_hour < 11 && !rs->morning_greeting_done) {
        rs->morning_greeting_done = true;
        callback(true, user_data);
    }
    /* Despedida Noturna: a partir das 23h até 4h */
    else if ((current_hour >= 23 || current_hour < 4) && !rs->night_farewell_done) {
        rs->night_farewell_done = true;
        callback(false, user_data);
    }
}
