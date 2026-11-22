#include "carmin_snapshot.h"

#include <math.h>
#include <stddef.h>

static uint64_t fnv1a_u64(uint64_t hash, uint64_t value) {
    for (unsigned i = 0; i < 8; ++i) {
        hash ^= (uint8_t)(value >> (i * 8u));
        hash *= 1099511628211ULL;
    }
    return hash;
}

bool carmin_snapshot_valid(const CarminTelemetrySnapshot *snapshot,
                           uint64_t now_ns, uint64_t max_age_ns) {
    if (!snapshot || snapshot->version != CARMIN_SNAPSHOT_VERSION ||
        snapshot->size != sizeof(*snapshot) || snapshot->timestamp_ns > now_ns ||
        now_ns - snapshot->timestamp_ns > max_age_ns ||
        snapshot->neuron_count == 0 || snapshot->region_count > 120 ||
        snapshot->active_neurons > snapshot->neuron_count) return false;
    for (uint16_t i = 0; i < snapshot->region_count; ++i)
        if (!isfinite(snapshot->region_rate[i]) || snapshot->region_rate[i] < 0.0f ||
            snapshot->region_rate[i] > 1.0f) return false;
    return true;
}

uint64_t carmin_group_map_hash(void) {
    uint64_t hash = 1469598103934665603ULL;
    hash = fnv1a_u64(hash, PESTINHA_CHANNELS);
    hash = fnv1a_u64(hash, PESTINHA_INTERNEURONS);
    hash = fnv1a_u64(hash, PESTINHA_MOTORS);
    hash = fnv1a_u64(hash, PESTINHA_NEURONS);
    hash = fnv1a_u64(hash, PESTINHA_NEURON_SCALE);
    return hash;
}

bool carmin_snapshot_capture(const double *pool_rates, uint32_t neuron_count,
                             uint32_t spike_count, uint64_t synapses_traversed,
                             uint32_t active_neurons, uint64_t sequence,
                             uint64_t timestamp_ns,
                             CarminTelemetrySnapshot *out_snapshot) {
    if (!pool_rates || !out_snapshot || neuron_count == 0) return false;
    CarminTelemetrySnapshot snapshot = {
        .version = CARMIN_SNAPSHOT_VERSION,
        .size = sizeof(CarminTelemetrySnapshot),
        .sequence = sequence,
        .timestamp_ns = timestamp_ns,
        .group_map_hash = carmin_group_map_hash(),
        .neuron_count = neuron_count,
        .spike_count = spike_count,
        .synapses_traversed = synapses_traversed,
        .active_neurons = active_neurons,
        .region_count = PESTINHA_CHANNELS + PESTINHA_INTERNEURONS + PESTINHA_MOTORS
    };
    for (uint16_t i = 0; i < snapshot.region_count; ++i) {
        if (!isfinite(pool_rates[i])) return false;
        float rate = (float)pool_rates[i];
        if (rate < 0.0f) rate = 0.0f;
        if (rate > 1.0f) rate = 1.0f;
        snapshot.region_rate[i] = rate;
    }
    *out_snapshot = snapshot;
    return true;
}
