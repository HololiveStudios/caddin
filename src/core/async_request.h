#ifndef CARMIN_ASYNC_REQUEST_H
#define CARMIN_ASYNC_REQUEST_H

#include <stdbool.h>
#include <stdint.h>

#define CARMIN_ASYNC_CAPACITY 16u
#define CARMIN_ASYNC_PAYLOAD_MAX 256u

typedef enum {
    CARMIN_ASYNC_EMPTY = 0,
    CARMIN_ASYNC_QUEUED,
    CARMIN_ASYNC_IN_FLIGHT,
    CARMIN_ASYNC_SUCCEEDED,
    CARMIN_ASYNC_FAILED,
    CARMIN_ASYNC_CANCELLED,
    CARMIN_ASYNC_EXPIRED
} CarminAsyncState;

typedef struct {
    uint64_t id;
    uint64_t deadline_ns;
    uint16_t payload_size;
    uint8_t state;
    uint8_t reserved;
    uint8_t payload[CARMIN_ASYNC_PAYLOAD_MAX];
} CarminAsyncRequest;

typedef struct {
    CarminAsyncRequest slots[CARMIN_ASYNC_CAPACITY];
    uint16_t head;
    uint16_t count;
    uint64_t next_id;
} CarminAsyncQueue;

void carmin_async_init(CarminAsyncQueue *queue);
bool carmin_async_submit(CarminAsyncQueue *queue, const void *payload, uint16_t payload_size,
                         uint64_t deadline_ns, uint64_t *out_id);
bool carmin_async_take(CarminAsyncQueue *queue, uint64_t now_ns, CarminAsyncRequest *out_request);
bool carmin_async_complete(CarminAsyncQueue *queue, uint64_t id, bool success,
                           uint64_t now_ns);
bool carmin_async_cancel(CarminAsyncQueue *queue, uint64_t id, uint64_t now_ns);
uint16_t carmin_async_expire(CarminAsyncQueue *queue, uint64_t now_ns);

#endif
