#ifndef CARMIN_SNAPSHOT_H
#define CARMIN_SNAPSHOT_H

#include <stdbool.h>
#include <stdint.h>

#include "runtime_types.h"

#define CARMIN_SNAPSHOT_VERSION 1u

typedef struct {
    uint32_t version;
    uint32_t size;
    uint64_t sequence;
    uint64_t timestamp_ns;
    uint64_t group_map_hash;
    uint32_t neuron_count;
    uint32_t spike_count;
    uint64_t synapses_traversed;
    uint32_t active_neurons;
    uint16_t region_count;
    uint16_t flags;
    float region_rate[120];
} CarminTelemetrySnapshot;

bool carmin_snapshot_valid(const CarminTelemetrySnapshot *snapshot,
                           uint64_t now_ns, uint64_t max_age_ns);
uint64_t carmin_group_map_hash(void);
bool carmin_snapshot_capture(const double *pool_rates, uint32_t neuron_count,
                             uint32_t spike_count, uint64_t synapses_traversed,
                             uint32_t active_neurons, uint64_t sequence,
                             uint64_t timestamp_ns,
                             CarminTelemetrySnapshot *out_snapshot);

#endif
