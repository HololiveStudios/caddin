#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "neuro.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

static uint64_t now_ns(void) {
#ifdef _WIN32
    LARGE_INTEGER counter, frequency;
    QueryPerformanceCounter(&counter);
    QueryPerformanceFrequency(&frequency);
    return (uint64_t)((long double)counter.QuadPart * 1000000000.0L / frequency.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
}

int main(int argc, char **argv) {
    int frames = argc > 1 ? atoi(argv[1]) : 30;
    const char *scenario = argc > 2 ? argv[2] : "medium";
    unsigned int seed = argc > 3 ? (unsigned int)strtoul(argv[3], NULL, 10) : 42u;
    if (frames < 1) frames = 1;
    Brain *b = brain_create(seed, "benchmark");
    if (!b) { fputs("brain_create failed\n", stderr); return 1; }
    double input[PESTINHA_CHANNELS] = {0};
    if (!strcmp(scenario, "low")) {
        input[CH_TYPING] = 0.2;
    } else if (!strcmp(scenario, "high")) {
        for (int c = 0; c < 8; ++c) input[c] = 1.0;
    } else if (!strcmp(scenario, "medium")) {
        input[CH_TYPING] = 0.8;
        input[CH_CPU_LOAD] = 0.3;
        input[CH_TALK] = 0.1;
    } else if (strcmp(scenario, "idle")) {
        fprintf(stderr, "scenario must be idle, low, medium or high\n");
        brain_destroy(b);
        return 2;
    }
    uint64_t total = 0, step_total = 0, stimulate_total = 0, learn_total = 0;
    uint64_t dense = (uint64_t)frames * 16u * PESTINHA_NEURONS;
    uint64_t updates = 0, sensory = 0, proposals = 0, sensory_spikes = 0, synapses = 0, spikes = 0;
    uint64_t sensory_ns = 0, propagation_ns = 0, integration_ns = 0;
#ifdef BRAIN_STEP_STATS_AVAILABLE
    BrainStepStats stats = {0};
#endif
    for (int i = 0; i < frames; ++i) {
        uint64_t frame_begin = now_ns();
        uint64_t phase = now_ns();
        brain_stimulate(b, input);
        uint64_t step_begin = now_ns();
        stimulate_total += step_begin - phase;
        brain_step(b, 16.0);
        uint64_t learn_begin = now_ns();
        step_total += learn_begin - step_begin;
        brain_learn_stdp(b, 0.1, 0.016);
        uint64_t frame_end = now_ns();
        learn_total += frame_end - learn_begin;
        total += frame_end - frame_begin;
#ifdef BRAIN_STEP_STATS_AVAILABLE
        (void)brain_step_stats(b, &stats);
        updates += stats.neuron_updates;
        sensory += stats.sensory_updates;
        proposals += stats.sensory_proposals;
        sensory_spikes += stats.sensory_spikes_emitted;
        sensory_ns += stats.sensory_ns;
        propagation_ns += stats.propagation_ns;
        integration_ns += stats.integration_ns;
        synapses += stats.synapses_traversed;
        spikes += stats.spikes_emitted;
#endif
    }
    printf("scenario=%s seed=%u frames=%d mean_core_ms=%.3f [stimulate=%.3f,step=%.3f,learn=%.3f] step_phases_ms[sensory=%.3f,propagation=%.3f,integration=%.3f] dense_updates=%llu integrated=%llu sensory_neuron_ticks=%llu sensory_proposals=%llu sensory_spikes=%llu synapses=%llu spikes=%llu synapse_total=%u\n",
           scenario, seed, frames, (double)total / frames / 1e6,
           (double)stimulate_total / frames / 1e6,
           (double)step_total / frames / 1e6,
           (double)learn_total / frames / 1e6,
           (double)sensory_ns / frames / 1e6,
           (double)propagation_ns / frames / 1e6,
           (double)integration_ns / frames / 1e6,
           (unsigned long long)dense,
           (unsigned long long)updates,
           (unsigned long long)sensory,
           (unsigned long long)proposals,
           (unsigned long long)sensory_spikes,
           (unsigned long long)synapses,
           (unsigned long long)spikes,
           (unsigned)brain_synapse_count(b));
    uint64_t state_hash = 1469598103934665603ULL;
    for (int i = 0; i < brain_pool_count(); ++i) {
        double rate = brain_pool_rate(b, i);
        unsigned char bytes[sizeof(rate)];
        memcpy(bytes, &rate, sizeof(rate));
        for (size_t k = 0; k < sizeof(bytes); ++k) state_hash = (state_hash ^ bytes[k]) * 1099511628211ULL;
    }
    for (int i = 0; i < PESTINHA_CHANNELS; ++i) {
        double efficacy = brain_efficacy(b, i);
        unsigned char bytes[sizeof(efficacy)];
        memcpy(bytes, &efficacy, sizeof(efficacy));
        for (size_t k = 0; k < sizeof(bytes); ++k) state_hash = (state_hash ^ bytes[k]) * 1099511628211ULL;
    }
    printf("observable_state_hash=%016llx last_substep_spikes=%d\n",
           (unsigned long long)state_hash, brain_active_spike_count(b));
#ifdef BRAIN_STEP_STATS_AVAILABLE
    printf("logical_full_state_hash_1e-9=%016llx\n",
           (unsigned long long)brain_bench_state_hash(b));
#endif
    brain_destroy(b);
    return 0;
}
