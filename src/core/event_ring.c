#include "event_ring.h"
#include <stdio.h>
#include <string.h>

void event_ring_init(EventRingBuffer *rb) {
    if (!rb) return;
    memset(rb, 0, sizeof(*rb));
}

void event_ring_push(EventRingBuffer *rb, EventType type, uint32_t tick, double now_s, float intensity, const char *tag) {
    if (!rb) return;

    TimedEvent *ev = &rb->events[rb->head];
    ev->type = type;
    ev->tick = tick;
    ev->timestamp_s = now_s;
    ev->intensity = intensity;
    if (tag) {
        snprintf(ev->tag, sizeof(ev->tag), "%s", tag);
    } else {
        ev->tag[0] = '\0';
    }

    rb->head = (rb->head + 1) % EVENT_RING_CAPACITY;
    if (rb->count < EVENT_RING_CAPACITY) {
        rb->count++;
    }
}

const TimedEvent *event_ring_get(const EventRingBuffer *rb, size_t index_from_latest) {
    if (!rb || index_from_latest >= rb->count) return NULL;
    size_t idx = (rb->head + EVENT_RING_CAPACITY - 1 - index_from_latest) % EVENT_RING_CAPACITY;
    return &rb->events[idx];
}

bool event_ring_has_recent(const EventRingBuffer *rb, EventType type, double now_s, double delta_s, float *out_intensity) {
    if (!rb || rb->count == 0) return false;

    for (size_t i = 0; i < rb->count; ++i) {
        const TimedEvent *ev = event_ring_get(rb, i);
        if (!ev) break;
        if (now_s - ev->timestamp_s > delta_s) {
            break; /* Eventos mais antigos que delta_s */
        }
        if (ev->type == type) {
            if (out_intensity) *out_intensity = ev->intensity;
            return true;
        }
    }
    return false;
}
