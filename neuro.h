#ifndef PESTINHA_NEURO_H
#define PESTINHA_NEURO_H

#include "runtime_types.h"

typedef struct Brain Brain;

typedef enum {
    BRAIN_POOL_SENSORY = 0,
    BRAIN_POOL_EXCITATORY = 1,
    BRAIN_POOL_INHIBITORY = 2,
    BRAIN_POOL_MOTOR = 3
} BrainPoolKind;

typedef struct {
    const char *name;
    uint32_t start;
    uint32_t size;
    BrainPoolKind kind;
} BrainPoolInfo;

Brain *brain_create(unsigned int seed, const char *temperament);
void brain_destroy(Brain *brain);
void brain_stimulate(Brain *brain, const double values[PESTINHA_CHANNELS]);
void brain_step(Brain *brain, double milliseconds);
double brain_motor(const Brain *brain, int motor_index);
double brain_interneuron(const Brain *brain, int index);
double brain_efficacy(const Brain *brain, int channel_index);
void brain_learn_stdp(Brain *brain, double reward_factor, double dt_sec);
void brain_reset_habituation(Brain *brain);
int brain_active_spike_count(const Brain *brain);
double brain_total_plastic_weight(const Brain *brain);

void brain_save(const Brain *brain, const char *path);
int brain_load(Brain *brain, const char *path);

const char *brain_channel_name(int index);
const char *brain_motor_name(int index);
const char *brain_interneuron_name(int index);

/* Ontogeny: acesso direto às taxas de disparo por pool e à variação plástica */
double brain_pool_rate(const Brain *brain, int pool_index);
int    brain_pool_count(void);  /* retorna NPOOLS */
bool   brain_pool_info(const Brain *brain, int pool_index, BrainPoolInfo *out);
uint32_t brain_synapse_count(const Brain *brain);

#endif /* PESTINHA_NEURO_H */
