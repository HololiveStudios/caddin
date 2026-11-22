#include "cognitive_event.h"

#include <math.h>
#include <string.h>

size_t carmin_events_encode_channels(
    const double channels[PESTINHA_CHANNELS],
    const CarminEventSource sources[PESTINHA_CHANNELS],
    const uint32_t flags[PESTINHA_CHANNELS],
    uint64_t first_sequence,
    uint64_t timestamp_ns,
    CarminEvent out[CARMIN_EVENT_BATCH_CAPACITY]) {
    if (!channels || !sources || !flags || !out) return 0;
    for (uint16_t channel = 0; channel < PESTINHA_CHANNELS; ++channel) {
        uint32_t status = flags[channel];
        bool unavailable = (status & CARMIN_EVENT_UNAVAILABLE) != 0 ||
                           (status & CARMIN_EVENT_AVAILABLE) == 0 || !isfinite(channels[channel]);
        if (unavailable) {
            status &= ~CARMIN_EVENT_AVAILABLE;
            status |= CARMIN_EVENT_UNAVAILABLE;
        }
        double value = unavailable ? 0.0 : channels[channel];
        if (value < -1.0) value = -1.0;
        if (value > 1.0) value = 1.0;
        out[channel] = (CarminEvent){
            .version = CARMIN_EVENT_VERSION,
            .size = (uint16_t)sizeof(CarminEvent),
            .sequence = first_sequence + channel,
            .timestamp_ns = timestamp_ns,
            .channel = channel,
            .source = (uint16_t)sources[channel],
            .flags = status,
            .value = (float)value
        };
    }
    return PESTINHA_CHANNELS;
}

bool carmin_event_is_valid(const CarminEvent *event, uint64_t now_ns, uint64_t max_age_ns) {
    if (!event || event->version != CARMIN_EVENT_VERSION || event->size != sizeof(*event) ||
        event->channel >= PESTINHA_CHANNELS || event->source > CARMIN_SOURCE_EVENT_DERIVED ||
        !isfinite(event->value) || event->value < -1.0f || event->value > 1.0f ||
        event->timestamp_ns > now_ns || now_ns - event->timestamp_ns > max_age_ns) return false;
    if ((event->flags & CARMIN_EVENT_AVAILABLE) && event->source == CARMIN_SOURCE_UNKNOWN) return false;
    if ((event->flags & CARMIN_EVENT_SENSITIVE) &&
        !(event->flags & (CARMIN_EVENT_REDACTED | CARMIN_EVENT_PRIVACY_BLOCKED))) return false;
    if ((event->flags & CARMIN_EVENT_AVAILABLE) && (event->flags & CARMIN_EVENT_UNAVAILABLE)) return false;
    return true;
}

bool carmin_events_decode_channels(
    const CarminEvent *events,
    size_t count,
    uint64_t now_ns,
    uint64_t max_age_ns,
    double channels[PESTINHA_CHANNELS]) {
    if ((!events && count) || !channels) return false;
    memset(channels, 0, sizeof(double) * PESTINHA_CHANNELS);
    uint64_t latest_sequence[PESTINHA_CHANNELS] = {0};
    bool seen[PESTINHA_CHANNELS] = {false};
    for (size_t i = 0; i < count; ++i) {
        const CarminEvent *event = &events[i];
        if (!carmin_event_is_valid(event, now_ns, max_age_ns)) continue;
        uint16_t channel = event->channel;
        if (seen[channel] && event->sequence < latest_sequence[channel]) continue;
        seen[channel] = true;
        latest_sequence[channel] = event->sequence;
        if (!(event->flags & CARMIN_EVENT_AVAILABLE) ||
            (event->flags & (CARMIN_EVENT_UNAVAILABLE | CARMIN_EVENT_PRIVACY_BLOCKED))) {
            channels[channel] = 0.0;
        } else {
            channels[channel] = fmax(0.0, fmin(1.0, event->value));
        }
    }
    return true;
}
