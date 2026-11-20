#ifndef PET_CORE_EVENT_RING_H
#define PET_CORE_EVENT_RING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define EVENT_RING_CAPACITY 256

typedef enum {
    EV_NONE = 0,
    EV_USER_TOUCH,      /* Carinho / toque no pet */
    EV_USER_SHAKE,      /* Sacudida violenta */
    EV_CODE_ERROR,      /* Erro de compilação/execução */
    EV_CODE_SUCCESS,    /* Build bem-sucedido */
    EV_HIGH_CPU,        /* Pico de carga de CPU */
    EV_FOCUS_SWITCH,    /* Troca de janela */
    EV_KEYSTROKE,       /* Digitação / tecla */
    EV_FATIGUE_HIGH,    /* Fadiga detectada */
    EV_INTERVENTION     /* Intervenção sugerida pelo mascote */
} EventType;

typedef struct {
    EventType type;
    uint32_t  tick;
    double    timestamp_s;
    float     intensity;
    char      tag[32];
} TimedEvent;

typedef struct {
    TimedEvent events[EVENT_RING_CAPACITY];
    size_t     head;
    size_t     count;
} EventRingBuffer;

void event_ring_init(EventRingBuffer *rb);
void event_ring_push(EventRingBuffer *rb, EventType type, uint32_t tick, double now_s, float intensity, const char *tag);
const TimedEvent *event_ring_get(const EventRingBuffer *rb, size_t index_from_latest);

/* Procura se ocorreu determinado tipo de evento nos últimos delta_s segundos */
bool event_ring_has_recent(const EventRingBuffer *rb, EventType type, double now_s, double delta_s, float *out_intensity);

#endif /* PET_CORE_EVENT_RING_H */
