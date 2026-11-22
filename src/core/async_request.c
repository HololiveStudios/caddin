#include "async_request.h"

#include <string.h>

void carmin_async_init(CarminAsyncQueue *queue) {
    if (!queue) return;
    memset(queue, 0, sizeof(*queue));
    queue->next_id = 1;
}

static bool terminal_state(uint8_t state) {
    return state == CARMIN_ASYNC_SUCCEEDED || state == CARMIN_ASYNC_FAILED ||
           state == CARMIN_ASYNC_CANCELLED || state == CARMIN_ASYNC_EXPIRED;
}

static void compact_queue(CarminAsyncQueue *queue) {
    if (!queue) return;
    CarminAsyncRequest retained[CARMIN_ASYNC_CAPACITY];
    uint16_t keep = 0;
    for (uint16_t i = 0; i < queue->count; ++i) {
        uint16_t index = (uint16_t)((queue->head + i) % CARMIN_ASYNC_CAPACITY);
        if (!terminal_state(queue->slots[index].state)) retained[keep++] = queue->slots[index];
    }
    memset(queue->slots, 0, sizeof(queue->slots));
    if (keep) memcpy(queue->slots, retained, (size_t)keep * sizeof(*retained));
    queue->head = 0;
    queue->count = keep;
}

bool carmin_async_submit(CarminAsyncQueue *queue, const void *payload, uint16_t payload_size,
                         uint64_t deadline_ns, uint64_t *out_id) {
    if (!queue || !payload || payload_size == 0 || payload_size > CARMIN_ASYNC_PAYLOAD_MAX ||
        deadline_ns == 0) return false;
    compact_queue(queue);
    if (queue->count >= CARMIN_ASYNC_CAPACITY) return false;
    uint16_t index = (uint16_t)((queue->head + queue->count) % CARMIN_ASYNC_CAPACITY);
    CarminAsyncRequest *request = &queue->slots[index];
    memset(request, 0, sizeof(*request));
    request->id = queue->next_id++;
    if (request->id == 0) request->id = queue->next_id++;
    request->deadline_ns = deadline_ns;
    request->payload_size = payload_size;
    request->state = CARMIN_ASYNC_QUEUED;
    memcpy(request->payload, payload, payload_size);
    queue->count++;
    if (out_id) *out_id = request->id;
    return true;
}

bool carmin_async_take(CarminAsyncQueue *queue, uint64_t now_ns, CarminAsyncRequest *out_request) {
    if (!queue || !out_request || queue->count == 0) return false;
    for (uint16_t i = 0; i < queue->count; ++i) {
        uint16_t index = (uint16_t)((queue->head + i) % CARMIN_ASYNC_CAPACITY);
        CarminAsyncRequest *request = &queue->slots[index];
        if (request->state == CARMIN_ASYNC_QUEUED && now_ns >= request->deadline_ns)
            request->state = CARMIN_ASYNC_EXPIRED;
        if (request->state == CARMIN_ASYNC_QUEUED) {
            request->state = CARMIN_ASYNC_IN_FLIGHT;
            *out_request = *request;
            compact_queue(queue);
            return true;
        }
    }
    compact_queue(queue);
    return false;
}

static CarminAsyncRequest *find_request(CarminAsyncQueue *queue, uint64_t id) {
    if (!queue || id == 0) return NULL;
    for (uint16_t i = 0; i < queue->count; ++i) {
        uint16_t index = (uint16_t)((queue->head + i) % CARMIN_ASYNC_CAPACITY);
        if (queue->slots[index].id == id) return &queue->slots[index];
    }
    return NULL;
}

bool carmin_async_complete(CarminAsyncQueue *queue, uint64_t id, bool success,
                           uint64_t now_ns) {
    CarminAsyncRequest *request = find_request(queue, id);
    if (!request || request->state != CARMIN_ASYNC_IN_FLIGHT) return false;
    request->state = now_ns >= request->deadline_ns ? CARMIN_ASYNC_EXPIRED :
                     (success ? CARMIN_ASYNC_SUCCEEDED : CARMIN_ASYNC_FAILED);
    return request->state != CARMIN_ASYNC_EXPIRED;
}

bool carmin_async_cancel(CarminAsyncQueue *queue, uint64_t id, uint64_t now_ns) {
    (void)now_ns;
    CarminAsyncRequest *request = find_request(queue, id);
    if (!request || (request->state != CARMIN_ASYNC_QUEUED &&
                     request->state != CARMIN_ASYNC_IN_FLIGHT)) return false;
    request->state = CARMIN_ASYNC_CANCELLED;
    return true;
}

uint16_t carmin_async_expire(CarminAsyncQueue *queue, uint64_t now_ns) {
    if (!queue) return 0;
    uint16_t expired = 0;
    for (uint16_t i = 0; i < queue->count; ++i) {
        uint16_t index = (uint16_t)((queue->head + i) % CARMIN_ASYNC_CAPACITY);
        CarminAsyncRequest *request = &queue->slots[index];
        if ((request->state == CARMIN_ASYNC_QUEUED || request->state == CARMIN_ASYNC_IN_FLIGHT) &&
            now_ns >= request->deadline_ns) {
            request->state = CARMIN_ASYNC_EXPIRED;
            expired++;
        }
    }
    return expired;
}
