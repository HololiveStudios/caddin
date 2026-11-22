#include "sdm.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum { PATTERN_COUNT = 24, QUERY_COUNT = 2048 };

static uint64_t now_ns(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static uint32_t next_rng(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static Hypervector noisy(Hypervector source, uint32_t *rng, unsigned flips) {
    unsigned changed = 0;
    while (changed < flips) {
        unsigned bit = next_rng(rng) % SDM_DIM_BITS;
        uint64_t mask = 1ULL << (bit % 64);
        source.words[bit / 64] ^= mask;
        changed++;
    }
    return source;
}

static void print_metrics(const char *label, unsigned queries, unsigned correct,
                          unsigned false_positives, uint64_t elapsed_ns,
                          uint32_t active_counters) {
    double ms = (double)elapsed_ns / 1.0e6;
    printf("%s queries=%u correct=%u accuracy=%.4f false_positive=%u latency_ms=%.3f "
           "us_per_query=%.3f active_counters=%" PRIu32 "\n",
           label, queries, correct, queries ? (double)correct / queries : 0.0,
           false_positives, ms, queries ? (double)elapsed_ns / queries / 1000.0 : 0.0,
           active_counters);
}

int main(int argc, char **argv) {
    unsigned replay_cycles = argc > 1 ? (unsigned)strtoul(argv[1], NULL, 10) : 4u;
    if (replay_cycles > 100u) replay_cycles = 100u;

    SparseDistributedMemory *memory = (SparseDistributedMemory *)malloc(sizeof(*memory));
    if (!memory) return 2;
    sdm_init(memory, 42);

    Hypervector addresses[PATTERN_COUNT];
    Hypervector outcomes[PATTERN_COUNT];
    uint32_t address_rng = 0xA551u;
    uint32_t outcome_rng = 0xF013u;
    for (unsigned i = 0; i < PATTERN_COUNT; ++i) {
        addresses[i] = hv_random(&address_rng);
        outcomes[i] = hv_random(&outcome_rng);
        /* O baseline is o palpite fixo modal: resultado zero, sem associação. */
    }

    uint64_t base_start = now_ns();
    unsigned baseline_correct = 0;
    for (unsigned q = 0; q < QUERY_COUNT; ++q) {
        unsigned pattern = q % PATTERN_COUNT;
        Hypervector fixed_guess = outcomes[0];
        baseline_correct += hv_similarity(fixed_guess, outcomes[pattern]) > 0.85f;
    }
    uint64_t base_elapsed = now_ns() - base_start;

    uint64_t empty_start = now_ns();
    unsigned empty_found = 0;
    for (unsigned q = 0; q < QUERY_COUNT; ++q) {
        Hypervector recalled;
        empty_found += sdm_recall(memory, addresses[q % PATTERN_COUNT], &recalled);
    }
    uint64_t empty_elapsed = now_ns() - empty_start;

    for (unsigned cycle = 0; cycle < replay_cycles; ++cycle) {
        for (unsigned i = 0; i < PATTERN_COUNT; ++i)
            sdm_store(memory, addresses[i], outcomes[i]);
    }

    uint64_t exact_start = now_ns();
    unsigned exact_correct = 0, exact_fp = 0;
    for (unsigned q = 0; q < QUERY_COUNT; ++q) {
        unsigned pattern = q % PATTERN_COUNT;
        Hypervector recalled;
        bool found = sdm_recall(memory, addresses[pattern], &recalled);
        float similarity = found ? hv_similarity(recalled, outcomes[pattern]) : -1.0f;
        exact_correct += similarity >= 0.70f;
        exact_fp += similarity < 0.70f;
    }
    uint64_t exact_elapsed = now_ns() - exact_start;

    uint64_t near_start = now_ns();
    unsigned near_correct = 0, near_fp = 0;
    uint32_t query_rng = 0x91E10DA5u;
    for (unsigned q = 0; q < QUERY_COUNT; ++q) {
        unsigned pattern = q % PATTERN_COUNT;
        Hypervector query = noisy(addresses[pattern], &query_rng, SDM_DIM_BITS / 32u);
        Hypervector recalled;
        bool found = sdm_recall(memory, query, &recalled);
        float similarity = found ? hv_similarity(recalled, outcomes[pattern]) : -1.0f;
        near_correct += similarity >= 0.60f;
        near_fp += similarity < 0.60f;
    }
    uint64_t near_elapsed = now_ns() - near_start;

    uint64_t interference_start = now_ns();
    unsigned interference_hits = 0, interference_false = 0;
    for (unsigned i = 0; i < PATTERN_COUNT; ++i) {
        uint32_t noise_rng = 0xD157u + i;
        Hypervector distractor = noisy(addresses[i], &noise_rng, SDM_DIM_BITS / 8u);
        sdm_store(memory, distractor, outcomes[(i + 1u) % PATTERN_COUNT]);
    }
    for (unsigned q = 0; q < QUERY_COUNT; ++q) {
        unsigned pattern = q % PATTERN_COUNT;
        Hypervector recalled;
        bool found = sdm_recall(memory, addresses[pattern], &recalled);
        float similarity = found ? hv_similarity(recalled, outcomes[pattern]) : -1.0f;
        interference_hits += similarity >= 0.60f;
        interference_false += similarity < 0.60f;
    }
    uint64_t interference_elapsed = now_ns() - interference_start;

    uint32_t before_sleep = sdm_active_counter_count(memory);
    unsigned active_query_hits = 0;
    uint64_t active_query_start = now_ns();
    for (unsigned q = 0; q < QUERY_COUNT; ++q) {
        Hypervector recalled;
        active_query_hits += sdm_recall(memory, addresses[q % PATTERN_COUNT], &recalled);
    }
    uint64_t active_query_elapsed = now_ns() - active_query_start;
    uint64_t sleep_start = now_ns();
    sdm_consolidate_sleep(memory);
    uint64_t sleep_elapsed = now_ns() - sleep_start;
    uint32_t after_sleep = sdm_active_counter_count(memory);

    uint64_t post_start = now_ns();
    unsigned post_correct = 0, post_fp = 0;
    for (unsigned q = 0; q < QUERY_COUNT; ++q) {
        unsigned pattern = q % PATTERN_COUNT;
        Hypervector recalled;
        bool found = sdm_recall(memory, addresses[pattern], &recalled);
        float similarity = found ? hv_similarity(recalled, outcomes[pattern]) : -1.0f;
        post_correct += similarity >= 0.70f;
        post_fp += similarity < 0.70f;
    }
    uint64_t post_elapsed = now_ns() - post_start;

    for (unsigned i = 0; i < PATTERN_COUNT; ++i) {
        outcomes[i] = (Hypervector){0};
        sdm_store(memory, addresses[i], outcomes[i]);
    }
    uint64_t reverse_start = now_ns();
    unsigned reversed_correct = 0, reversed_fp = 0;
    for (unsigned q = 0; q < QUERY_COUNT; ++q) {
        unsigned pattern = q % PATTERN_COUNT;
        Hypervector recalled;
        bool found = sdm_recall(memory, addresses[pattern], &recalled);
        float similarity = found ? hv_similarity(recalled, outcomes[pattern]) : -1.0f;
        reversed_correct += similarity >= 0.60f;
        reversed_fp += similarity < 0.60f;
    }
    uint64_t reverse_elapsed = now_ns() - reverse_start;

    printf("sdm seed=42 locations=%d active_k=%d dimension_bits=%d patterns=%d replay_cycles=%u "
           "reversal=zero_outcome query_seed=0x91e10da5 thresholds=exact:0.70,near:0.60\n",
           SDM_LOCATIONS, SDM_K_ACTIVE, SDM_DIM_BITS, PATTERN_COUNT, replay_cycles);
    print_metrics("baseline_fixed_guess", QUERY_COUNT, baseline_correct, QUERY_COUNT - baseline_correct,
                  base_elapsed, 0);
    print_metrics("empty_memory", QUERY_COUNT, empty_found, QUERY_COUNT - empty_found,
                  empty_elapsed, 0);
    print_metrics("active_memory_recall", QUERY_COUNT, active_query_hits,
                  QUERY_COUNT - active_query_hits, active_query_elapsed, before_sleep);
    print_metrics("learned_exact", QUERY_COUNT, exact_correct, exact_fp, exact_elapsed,
                  sdm_active_counter_count(memory));
    print_metrics("learned_6.25pct_noise", QUERY_COUNT, near_correct, near_fp, near_elapsed,
                  sdm_active_counter_count(memory));
    print_metrics("after_interference", QUERY_COUNT, interference_hits, interference_false,
                  interference_elapsed, sdm_active_counter_count(memory));
    print_metrics("after_sleep_replay", QUERY_COUNT, post_correct, post_fp, post_elapsed,
                  after_sleep);
    printf("consolidation active_before=%" PRIu32 " active_after=%" PRIu32
           " latency_ms=%.3f\n", before_sleep, after_sleep, (double)sleep_elapsed / 1.0e6);
    print_metrics("reversal_to_zero", QUERY_COUNT, reversed_correct, reversed_fp,
                  reverse_elapsed, after_sleep);

    free(memory);
    return 0;
}
