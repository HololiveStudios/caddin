#include "async_request.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define NS_PER_SECOND 1000000000ULL

int main(void) {
    CarminAsyncQueue queue;
    carmin_async_init(&queue);
    const char payload[] = "structured-task-id-17";
    uint64_t first_id = 0, second_id = 0;
    assert(carmin_async_submit(&queue, payload, sizeof(payload), NS_PER_SECOND, &first_id));
    assert(carmin_async_submit(&queue, payload, sizeof(payload), 2 * NS_PER_SECOND, &second_id));
    CarminAsyncRequest request;
    assert(carmin_async_take(&queue, NS_PER_SECOND / 10, &request));
    assert(request.id == first_id && request.state == CARMIN_ASYNC_IN_FLIGHT);
    assert(carmin_async_cancel(&queue, first_id, NS_PER_SECOND / 10 + 1));
    assert(!carmin_async_complete(&queue, first_id, true, NS_PER_SECOND / 10 + 2));
    assert(carmin_async_take(&queue, NS_PER_SECOND / 10, &request));
    assert(request.id == second_id);
    assert(carmin_async_complete(&queue, second_id, true, NS_PER_SECOND / 10 + 50));
    assert(!carmin_async_complete(&queue, second_id, true, NS_PER_SECOND / 10 + 51));

    uint64_t expired_id = 0;
    assert(carmin_async_submit(&queue, payload, sizeof(payload), 300, &expired_id));
    assert(carmin_async_expire(&queue, 300) == 1);
    assert(!carmin_async_take(&queue, 301, &request));

    uint64_t late_id = 0;
    assert(carmin_async_submit(&queue, payload, sizeof(payload), 400, &late_id));
    assert(!carmin_async_take(&queue, 400, &request));

    for (unsigned i = 0; i < CARMIN_ASYNC_CAPACITY; ++i)
        assert(carmin_async_submit(&queue, payload, sizeof(payload), 5 * NS_PER_SECOND + i, NULL));
    assert(!carmin_async_submit(&queue, payload, sizeof(payload), 9 * NS_PER_SECOND, NULL));
    assert(!carmin_async_submit(&queue, payload, CARMIN_ASYNC_PAYLOAD_MAX + 1,
                                9 * NS_PER_SECOND, NULL));
    puts("Async Request: OK");
    return 0;
}
