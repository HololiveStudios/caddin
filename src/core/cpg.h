#ifndef PET_CPG_H
#define PET_CPG_H

#include "fixed.h"
#include <stdbool.h>

typedef struct {
    uint16_t phase;          /* Fase oscilatória [0..65535] */
    uint16_t base_freq;      /* Frequência base de respiração */
    fix16_t  current_depth;  /* Amplitude/profundidade da respiração Q8.8 */

    /* Febre cognitiva (proteção anti-saturação) */
    uint32_t spike_window_accum;
    uint16_t window_ticks;
    bool     blackout_active;
    uint16_t blackout_remaining_ticks;
} CentralPatternGenerator;

void    cpg_init(CentralPatternGenerator *cpg);
void    cpg_step(CentralPatternGenerator *cpg, fix16_t arousal_stress, uint32_t current_spikes, uint32_t total_neurons);
fix16_t cpg_get_breath_expansion(const CentralPatternGenerator *cpg);
bool    cpg_is_in_blackout(const CentralPatternGenerator *cpg);

#endif /* PET_CPG_H */
