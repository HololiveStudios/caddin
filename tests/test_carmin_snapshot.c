#include "carmin_snapshot.h"

#include <assert.h>
#include <stdio.h>

int main(void) {
    CarminTelemetrySnapshot snapshot = {
        .version = CARMIN_SNAPSHOT_VERSION,
        .size = sizeof(CarminTelemetrySnapshot),
        .sequence = 1,
        .timestamp_ns = 100,
        .group_map_hash = 0x1234,
        .neuron_count = 1048576,
        .spike_count = 12,
        .synapses_traversed = 768,
        .active_neurons = 64,
        .region_count = 120
    };
    snapshot.region_rate[119] = 0.5f;
    assert(carmin_snapshot_valid(&snapshot, 200, 100));
    double rates[120] = {0};
    CarminTelemetrySnapshot captured;
    rates[119] = 0.5;
    assert(carmin_snapshot_capture(rates, 1048576, 12, 768, 64, 2, 150, &captured));
    assert(captured.group_map_hash == carmin_group_map_hash());
    assert(captured.region_count == 120 && captured.region_rate[119] == 0.5f);
    assert(!carmin_snapshot_valid(&snapshot, 201, 100));
    snapshot.region_rate[0] = 1.5f;
    assert(!carmin_snapshot_valid(&snapshot, 200, 100));
    snapshot.region_rate[0] = 0.0f;
    snapshot.active_neurons = snapshot.neuron_count + 1;
    assert(!carmin_snapshot_valid(&snapshot, 200, 100));
    puts("Carmin Snapshot: OK");
    return 0;
}
