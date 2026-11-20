#ifndef PET_CORE_RITUALS_H
#define PET_CORE_RITUALS_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t last_milestone_hours;
    bool     morning_greeting_done;
    bool     night_farewell_done;
    int      last_checked_day;
} RitualState;

void rituals_init(RitualState *rs);

typedef void (*MilestoneCallback)(uint32_t hours, void *user_data);
typedef void (*CircadianCallback)(bool is_morning, void *user_data);

/* Verifica se o tempo acumulado acordado atingiu um novo marco (1h, 10h, 50h, 100h, etc) */
void rituals_check_milestones(RitualState *rs,
                              uint32_t awake_seconds,
                              MilestoneCallback callback,
                              void *user_data);

/* Verifica saudações circadianas ("Bom dia" matinal e "Boa noite" noturno) */
void rituals_check_circadian(RitualState *rs,
                             int current_hour,
                             int current_day,
                             CircadianCallback callback,
                             void *user_data);

#endif /* PET_CORE_RITUALS_H */
