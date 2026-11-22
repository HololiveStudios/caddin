#ifndef CARMIN_COGNITIVE_EVENT_H
#define CARMIN_COGNITIVE_EVENT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "runtime_types.h"

#define CARMIN_EVENT_VERSION 1u
#define CARMIN_EVENT_BATCH_CAPACITY PESTINHA_CHANNELS

typedef enum {
    CARMIN_SOURCE_UNKNOWN = 0,
    CARMIN_SOURCE_APP_ENCODER,
    CARMIN_SOURCE_SYSTEM_MONITOR,
    CARMIN_SOURCE_MICROPHONE,
    CARMIN_SOURCE_WINDOW_METADATA,
    CARMIN_SOURCE_INTERNAL_STATE,
    CARMIN_SOURCE_EVENT_DERIVED
} CarminEventSource;

enum {
    CARMIN_EVENT_AVAILABLE       = 1u << 0,
    CARMIN_EVENT_UNAVAILABLE     = 1u << 1,
    CARMIN_EVENT_SENSITIVE       = 1u << 2,
    CARMIN_EVENT_REDACTED        = 1u << 3,
    CARMIN_EVENT_PRIVACY_BLOCKED = 1u << 4,
    CARMIN_EVENT_SYNTHETIC       = 1u << 5
};

typedef struct {
    uint16_t version;
    uint16_t size;
    uint64_t sequence;
    uint64_t timestamp_ns;
    uint16_t channel;
    uint16_t source;
    uint32_t flags;
    float value;
} CarminEvent;

/* Scalar-only event batch. Raw text, audio and screen pixels are never carried here. */
size_t carmin_events_encode_channels(
    const double channels[PESTINHA_CHANNELS],
    const CarminEventSource sources[PESTINHA_CHANNELS],
    const uint32_t flags[PESTINHA_CHANNELS],
    uint64_t first_sequence,
    uint64_t timestamp_ns,
    CarminEvent out[CARMIN_EVENT_BATCH_CAPACITY]);

bool carmin_event_is_valid(const CarminEvent *event, uint64_t now_ns, uint64_t max_age_ns);

/* Latest sequence wins per channel; stale, unavailable and privacy-blocked values become zero. */
bool carmin_events_decode_channels(
    const CarminEvent *events,
    size_t count,
    uint64_t now_ns,
    uint64_t max_age_ns,
    double channels[PESTINHA_CHANNELS]);

#endif
