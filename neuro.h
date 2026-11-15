#ifndef PESTINHA_NEURO_H
#define PESTINHA_NEURO_H

#include "runtime_types.h"

typedef struct Brain Brain;

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

#endif /* PESTINHA_NEURO_H */
