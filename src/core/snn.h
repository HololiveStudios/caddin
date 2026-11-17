#ifndef PET_SNN_H
#define PET_SNN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "fixed.h"
#include "arena.h"

#define SNN_MAX_DELAY_TICKS 8
#define SNN_DEFAULT_THR     fix_from_float(1.0f)
#define SNN_RESET_V         fix_from_float(0.0f)
#define SNN_REST_V          fix_from_float(0.0f)

typedef struct {
    uint32_t n_neurons;
    uint32_t n_synapses;

    /* Neurônios (SoA em ponto fixo) */
    fix16_t *v;           /* Potencial de membrana Q8.8 */
    fix16_t *thr;         /* Limiar adaptativo Q8.8 */
    uint8_t *refrac;      /* Ticks restantes de período refratário */
    fix16_t *i_in;        /* Corrente de entrada acumulada neste tick */
    uint8_t *region;      /* ID da região/módulo funcional */
    uint8_t *spiked;      /* 1 se disparou no tick atual, 0 caso contrário */
    fix16_t *pre_trace;   /* Traço STDP pré-sináptico */
    fix16_t *post_trace;  /* Traço STDP pós-sináptico */

    /* Sinapses em formato CSR */
    uint32_t *row_ptr;    /* n_neurons + 1 */
    uint32_t *post;       /* n_synapses */
    fix16_t  *w;          /* Pesos sinápticos Q8.8 */
    fix16_t  *w0;         /* Pesos basais originais para homeostase BCM */
    uint8_t  *delay;      /* Atraso em ticks (0..7) */
    fix16_t  *elig;       /* Traço de elegibilidade (R-STDP) */
    uint8_t  *stp_u;      /* STP Tsodyks-Markram: fração de liberação (Q0.8) */
    uint8_t  *stp_x;      /* STP Tsodyks-Markram: recursos disponíveis (Q0.8) */

    /* Buffer circular de retardo para entrega de correntes sinápticas */
    /* [SNN_MAX_DELAY_TICKS][n_neurons] */
    fix16_t  *delay_buffer;
    uint32_t  delay_head;

    /* Fila e estatísticas de spikes */
    uint32_t *spike_queue;
    uint32_t  spike_count;
    uint32_t  total_spikes_lifetime;

    int16_t leak_shift;   /* Decaimento exponencial: v -= v >> leak_shift */
    uint64_t current_tick;
} SnnNetwork;

/* Inicialização da rede na Arena */
bool snn_init(SnnNetwork *snn, Arena *arena, uint32_t n_neurons, uint32_t max_synapses, int16_t leak_shift);

/* Adição de sinapses antes de selar a topologia */
bool snn_add_synapse(SnnNetwork *snn, uint32_t pre, uint32_t post, fix16_t weight, uint8_t delay);

/* Injeção de corrente externa */
void snn_inject_current(SnnNetwork *snn, uint32_t neuron_idx, fix16_t current);

/* Execução de 1 tick da rede (Zero alocações) */
void snn_step(SnnNetwork *snn);

/* Aplicação de R-STDP e plasticidade de curto prazo (STP) */
void snn_apply_plasticity(SnnNetwork *snn, fix16_t dopamine_delta, fix16_t learning_rate);

/* Consultas */
bool     snn_is_spiking(const SnnNetwork *snn, uint32_t neuron_idx);
uint32_t snn_get_spike_count(const SnnNetwork *snn);
fix16_t  snn_get_voltage(const SnnNetwork *snn, uint32_t neuron_idx);

#endif /* PET_SNN_H */
