#include "cognitive_event.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void test_batch_round_trip_and_privacy(void) {
    double inputs[PESTINHA_CHANNELS] = {0};
    CarminEventSource sources[PESTINHA_CHANNELS];
    uint32_t flags[PESTINHA_CHANNELS];
    CarminEvent events[CARMIN_EVENT_BATCH_CAPACITY];
    double decoded[PESTINHA_CHANNELS];
    for (int i = 0; i < PESTINHA_CHANNELS; ++i) {
        sources[i] = CARMIN_SOURCE_APP_ENCODER;
        flags[i] = CARMIN_EVENT_AVAILABLE;
    }
    inputs[CH_TYPING] = 0.75;
    inputs[CH_KEY_FAST] = -0.25;
    inputs[CH_CPU_LOAD] = 1.25;
    flags[CH_MUSIC] = CARMIN_EVENT_UNAVAILABLE;
    sources[CH_MUSIC] = CARMIN_SOURCE_UNKNOWN;
    flags[CH_NOISE] |= CARMIN_EVENT_PRIVACY_BLOCKED;

    size_t count = carmin_events_encode_channels(inputs, sources, flags, 1000, 5000000, events);
    assert(count == PESTINHA_CHANNELS);
    assert(events[CH_TYPING].version == CARMIN_EVENT_VERSION);
    assert(events[CH_TYPING].sequence == 1000 + CH_TYPING);
    assert(events[CH_TYPING].timestamp_ns == 5000000);
    assert(carmin_events_decode_channels(events, count, 5000000, 0, decoded));
    assert(fabs(decoded[CH_TYPING] - 0.75) < 1e-6);
    assert(decoded[CH_KEY_FAST] == 0.0);
    assert(decoded[CH_CPU_LOAD] == 1.0);
    assert(decoded[CH_MUSIC] == 0.0);
    assert(decoded[CH_NOISE] == 0.0);
}

static void test_validation_freshness_and_latest_sequence(void) {
    CarminEvent event = {
        .version = CARMIN_EVENT_VERSION,
        .size = (uint16_t)sizeof(CarminEvent),
        .sequence = 10,
        .timestamp_ns = 100,
        .channel = CH_TYPING,
        .source = CARMIN_SOURCE_APP_ENCODER,
        .flags = CARMIN_EVENT_AVAILABLE,
        .value = 0.5f
    };
    assert(carmin_event_is_valid(&event, 110, 10));
    assert(!carmin_event_is_valid(&event, 111, 10));
    assert(!carmin_event_is_valid(&event, 99, 10));

    CarminEvent batch[2] = {event, event};
    batch[0].sequence = 12;
    batch[0].value = 0.8f;
    batch[1].sequence = 11;
    batch[1].value = 0.2f;
    double decoded[PESTINHA_CHANNELS];
    assert(carmin_events_decode_channels(batch, 2, 100, 0, decoded));
    assert(fabs(decoded[CH_TYPING] - 0.8) < 1e-6);

    event.flags |= CARMIN_EVENT_SENSITIVE;
    assert(!carmin_event_is_valid(&event, 100, 0));
    event.flags |= CARMIN_EVENT_REDACTED;
    assert(carmin_event_is_valid(&event, 100, 0));
    event.version++;
    assert(!carmin_event_is_valid(&event, 100, 0));
    event.version = CARMIN_EVENT_VERSION;
    event.channel = PESTINHA_CHANNELS;
    assert(!carmin_event_is_valid(&event, 100, 0));
    event.channel = CH_TYPING;
    event.value = NAN;
    assert(!carmin_event_is_valid(&event, 100, 0));
}

int main(void) {
    test_batch_round_trip_and_privacy();
    test_validation_freshness_and_latest_sequence();
    puts("Cognitive event contract: OK");
    return 0;
}
