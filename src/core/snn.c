#include "snn.h"
#include <string.h>

#define STDP_TRACE_SHIFT 4  /* Decaimento exponencial dos traços pré/pós */
#define ELIG_DECAY_SHIFT 5  /* Decaimento do traço de elegibilidade */
#define STP_RECOVERY_RATE 4 /* Taxa de reposição de neurotransmissores STP */

bool snn_init(SnnNetwork *snn, Arena *arena, uint32_t n_neurons, uint32_t max_synapses, int16_t leak_shift) {
    if (!snn || !arena || n_neurons == 0) return false;

    memset(snn, 0, sizeof(SnnNetwork));
    snn->n_neurons = n_neurons;
    snn->leak_shift = (leak_shift > 0) ? leak_shift : 3;

    /* Alocar neurônios */
    snn->v = (fix16_t *)arena_alloc_zero(arena, n_neurons * sizeof(fix16_t));
    snn->thr = (fix16_t *)arena_alloc(arena, n_neurons * sizeof(fix16_t));
    snn->refrac = (uint8_t *)arena_alloc_zero(arena, n_neurons * sizeof(uint8_t));
    snn->i_in = (fix16_t *)arena_alloc_zero(arena, n_neurons * sizeof(fix16_t));
    snn->region = (uint8_t *)arena_alloc_zero(arena, n_neurons * sizeof(uint8_t));
    snn->spiked = (uint8_t *)arena_alloc_zero(arena, n_neurons * sizeof(uint8_t));
    snn->pre_trace = (fix16_t *)arena_alloc_zero(arena, n_neurons * sizeof(fix16_t));
    snn->post_trace = (fix16_t *)arena_alloc_zero(arena, n_neurons * sizeof(fix16_t));

    /* Inicializar limiar padrão */
    for (uint32_t i = 0; i < n_neurons; ++i) {
        snn->thr[i] = SNN_DEFAULT_THR;
    }

    /* Alocar sinapses CSR */
    snn->row_ptr = (uint32_t *)arena_alloc_zero(arena, (n_neurons + 1) * sizeof(uint32_t));
    snn->post = (uint32_t *)arena_alloc_zero(arena, max_synapses * sizeof(uint32_t));
    snn->w = (fix16_t *)arena_alloc_zero(arena, max_synapses * sizeof(fix16_t));
    snn->w0 = (fix16_t *)arena_alloc_zero(arena, max_synapses * sizeof(fix16_t));
    snn->delay = (uint8_t *)arena_alloc_zero(arena, max_synapses * sizeof(uint8_t));
    snn->elig = (fix16_t *)arena_alloc_zero(arena, max_synapses * sizeof(fix16_t));
    snn->stp_u = (uint8_t *)arena_alloc_zero(arena, max_synapses * sizeof(uint8_t));
    snn->stp_x = (uint8_t *)arena_alloc_zero(arena, max_synapses * sizeof(uint8_t));

    for (uint32_t i = 0; i < max_synapses; ++i) {
        snn->stp_u[i] = 200;  /* Q0.8 ~ 0.78 taxa basal de liberação */
        snn->stp_x[i] = 255;  /* Q0.8 ~ 1.00 reservatório cheio */
    }

    /* Buffer circular de retardo */
    snn->delay_buffer = (fix16_t *)arena_alloc_zero(arena, SNN_MAX_DELAY_TICKS * n_neurons * sizeof(fix16_t));
    snn->delay_head = 0;

    /* Fila de spikes */
    snn->spike_queue = (uint32_t *)arena_alloc_zero(arena, n_neurons * sizeof(uint32_t));
    snn->spike_count = 0;
    snn->total_spikes_lifetime = 0;
    snn->current_tick = 0;

    return (snn->delay_buffer != NULL);
}

bool snn_add_synapse(SnnNetwork *snn, uint32_t pre, uint32_t post, fix16_t weight, uint8_t delay) {
    if (!snn || pre >= snn->n_neurons || post >= snn->n_neurons) return false;

    uint32_t idx = snn->n_synapses;
    snn->post[idx] = post;
    snn->w[idx] = weight;
    snn->w0[idx] = weight;
    snn->delay[idx] = (delay < SNN_MAX_DELAY_TICKS) ? delay : (SNN_MAX_DELAY_TICKS - 1);
    snn->elig[idx] = 0;
    snn->n_synapses++;

    /* Atualizar ponteiros de linha CSR incrementalmente */
    for (uint32_t i = pre + 1; i <= snn->n_neurons; ++i) {
        snn->row_ptr[i] = snn->n_synapses;
    }
    return true;
}

void snn_inject_current(SnnNetwork *snn, uint32_t neuron_idx, fix16_t current) {
    if (snn && neuron_idx < snn->n_neurons) {
        snn->i_in[neuron_idx] = sat_add16(snn->i_in[neuron_idx], current);
    }
}

void snn_step(SnnNetwork *snn) {
    if (!snn) return;

    snn->spike_count = 0;
    uint32_t n = snn->n_neurons;
    uint32_t head = snn->delay_head;
    fix16_t *current_delay_slot = &snn->delay_buffer[head * n];

    /* 1. Integração e detecção de picos */
    for (uint32_t i = 0; i < n; ++i) {
        snn->spiked[i] = 0;

        /* Decaimento dos traços STDP */
        snn->pre_trace[i] = fix_decay_shift(snn->pre_trace[i], STDP_TRACE_SHIFT);
        snn->post_trace[i] = fix_decay_shift(snn->post_trace[i], STDP_TRACE_SHIFT);

        if (snn->refrac[i] > 0) {
            snn->refrac[i]--;
            snn->v[i] = SNN_RESET_V;
            snn->i_in[i] = 0;
            current_delay_slot[i] = 0;
            continue;
        }

        /* Corrente total = entrada externa + chegada sináptica */
        fix16_t total_i = sat_add16(snn->i_in[i], current_delay_slot[i]);
        current_delay_slot[i] = 0; /* Limpar slot consumido */
        snn->i_in[i] = 0;

        /* Potencial com decaimento exponencial: v = v - (v >> leak_shift) + I */
        fix16_t decayed_v = fix_decay_shift(snn->v[i], snn->leak_shift);
        snn->v[i] = sat_add16(decayed_v, total_i);

        /* Checar disparo de spike */
        if (snn->v[i] >= snn->thr[i]) {
            snn->spiked[i] = 1;
            snn->v[i] = SNN_RESET_V;
            snn->refrac[i] = 2; /* 2 ticks de período refratário */
            snn->pre_trace[i] = sat_add16(snn->pre_trace[i], FIX_ONE);

            /* Registrar na fila de spikes */
            snn->spike_queue[snn->spike_count++] = i;
            snn->total_spikes_lifetime++;
        }
    }

    /* 2. Propagação sináptica CSR e fadiga STP */
    for (uint32_t s = 0; s < snn->spike_count; ++s) {
        uint32_t pre = snn->spike_queue[s];
        uint32_t start = snn->row_ptr[pre];
        uint32_t end = snn->row_ptr[pre + 1];

        for (uint32_t syn = start; syn < end; ++syn) {
            uint32_t post = snn->post[syn];
            uint8_t d = snn->delay[syn];
            uint32_t target_slot = (head + d) % SNN_MAX_DELAY_TICKS;

            /* Dinâmica STP Tsodyks-Markram */
            /* Peso efetivo = w * (u * x) */
            uint8_t u = snn->stp_u[syn];
            uint8_t x = snn->stp_x[syn];
            fix16_t transmission_factor = (fix16_t)(((uint32_t)u * (uint32_t)x) >> 8);
            fix16_t effective_weight = fix_mul(snn->w[syn], transmission_factor);

            /* Entrega de corrente no slot futuro */
            fix16_t *target_slot_ptr = &snn->delay_buffer[target_slot * n];
            target_slot_ptr[post] = sat_add16(target_slot_ptr[post], effective_weight);

            /* Consumo de neurotransmissor (depressão): x -= u * x */
            uint8_t consumed = (uint8_t)(((uint32_t)u * (uint32_t)x) >> 8);
            snn->stp_x[syn] = (x > consumed) ? (x - consumed) : 0;

            /* Atualização do traço de elegibilidade STDP */
            /* elegibilidade += post_trace - pre_trace (aproximação pareada) */
            fix16_t timing_diff = sat_sub16(snn->post_trace[post], snn->pre_trace[pre]);
            snn->elig[syn] = sat_add16(snn->elig[syn], timing_diff);
        }
    }

    /* 3. Recuperação passiva de recursos STP nas sinapses */
    for (uint32_t syn = 0; syn < snn->n_synapses; ++syn) {
        if (snn->stp_x[syn] < 255) {
            uint16_t recovered = snn->stp_x[syn] + STP_RECOVERY_RATE;
            snn->stp_x[syn] = (recovered > 255) ? 255 : (uint8_t)recovered;
        }
        /* Decaimento contínuo do traço de elegibilidade */
        snn->elig[syn] = fix_decay_shift(snn->elig[syn], ELIG_DECAY_SHIFT);
    }

    /* Avançar cabeça do buffer circular de atraso */
    snn->delay_head = (snn->delay_head + 1) % SNN_MAX_DELAY_TICKS;
    snn->current_tick++;
}

void snn_apply_plasticity(SnnNetwork *snn, fix16_t dopamine_delta, fix16_t learning_rate) {
    if (!snn) return;

    /* R-STDP Trifatorial: delta_w = lr * elig * dopamine_delta */
    for (uint32_t syn = 0; syn < snn->n_synapses; ++syn) {
        if (snn->elig[syn] == 0) continue;

        fix16_t mod = fix_mul(snn->elig[syn], dopamine_delta);
        fix16_t delta_w = fix_mul(mod, learning_rate);
        snn->w[syn] = sat_add16(snn->w[syn], delta_w);

        /* Homeostase passiva BCM: leve relaxamento em direção ao peso original w0 */
        fix16_t diff_w0 = sat_sub16(snn->w0[syn], snn->w[syn]);
        snn->w[syn] = sat_add16(snn->w[syn], diff_w0 >> 10);
    }
}

bool snn_is_spiking(const SnnNetwork *snn, uint32_t neuron_idx) {
    return (snn && neuron_idx < snn->n_neurons) ? (snn->spiked[neuron_idx] != 0) : false;
}

uint32_t snn_get_spike_count(const SnnNetwork *snn) {
    return snn ? snn->spike_count : 0;
}

fix16_t snn_get_voltage(const SnnNetwork *snn, uint32_t neuron_idx) {
    return (snn && neuron_idx < snn->n_neurons) ? snn->v[neuron_idx] : 0;
}
