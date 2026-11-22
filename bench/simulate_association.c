#include "sdm.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

enum { TASKS = 24, FRAMES = 720, SWITCH_AT = 360 };

static uint64_t hash_bytes(uint64_t hash, const void *data, size_t size) {
    const unsigned char *p = data;
    for (size_t i = 0; i < size; ++i) {
        hash ^= p[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

int main(void) {
    SdmEpisodeIndex *memory = malloc(sizeof(*memory));
    if (!memory) return 2;
    sdm_episode_index_init(memory);
    Hypervector contexts[TASKS], outcomes[TASKS];
    uint32_t context_rng = 0xC07E57u, outcome_rng = 0x0B1EC7u;
    for (unsigned i = 0; i < TASKS; ++i) {
        contexts[i] = hv_random(&context_rng);
        outcomes[i] = hv_random(&outcome_rng);
    }

    unsigned attempts = 0, correct = 0, empty = 0, stale = 0;
    uint64_t replay_hash = 1469598103934665603ULL;
    for (unsigned frame = 0; frame < FRAMES; ++frame) {
        unsigned task = (frame / 30u) % TASKS;
        if (frame == SWITCH_AT) {
            /* Reversal event: retarget all task contexts to fresh outcomes. */
            sdm_episode_index_consolidate(memory);
            replay_hash = hash_bytes(replay_hash, &frame, sizeof(frame));
            replay_hash = hash_bytes(replay_hash, &memory->count, sizeof(memory->count));
            outcome_rng = 0x5E7E25u;
            for (unsigned i = 0; i < TASKS; ++i) outcomes[i] = hv_random(&outcome_rng);
        }
        if (frame == 719u) {
            sdm_episode_index_consolidate(memory); /* replay/sleep após fechar cada bloco */
            replay_hash = hash_bytes(replay_hash, &frame, sizeof(frame));
            replay_hash = hash_bytes(replay_hash, &memory->count, sizeof(memory->count));
        }

        SdmEpisodeRecall recalled;
        sdm_episode_index_recall(memory, contexts[task], &recalled);
        if (!recalled.valid) empty++;
        else if (recalled.similarity >= 0.99f && hv_hamming_dist(recalled.value, outcomes[task]) == 0) correct++;
        else stale++;
        attempts++;

        if (frame % 30u == 29u || frame % 30u == 0 || frame == SWITCH_AT + 1u)
            (void)sdm_episode_index_store(memory, contexts[task], outcomes[task]);
    }

    printf("simulation=virtual_clock_association seed=42 frames=%u switch_at=%u tasks=%u "
           "attempts=%u correct=%u stale=%u empty=%u accuracy=%.4f active_episodes=%" PRIu32
           " replay_state_count=%" PRIu32 " replay_hash=%016" PRIx64 "\n",
           FRAMES, SWITCH_AT, TASKS, attempts, correct, stale, empty,
           attempts ? (double)correct / attempts : 0.0,
           memory->count, memory->count, replay_hash);
    free(memory);
    return 0;
}
