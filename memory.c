#include "memory.h"
#include <string.h>

void memory_init(EpisodeMemory *m) {
    if (!m) return;
    memset(m, 0, sizeof(*m));
}

void memory_record(EpisodeMemory *m, const char *type, double now_t) {
    if (!m || !type) return;

    if (!strcmp(type, "caress")) {
        m->last_caress_t = now_t;
        m->positive_interactions++;
    } else if (!strcmp(type, "poke")) {
        m->last_poke_t = now_t;
        m->negative_interactions++;
    } else if (!strcmp(type, "feed")) {
        m->last_feed_t = now_t;
        m->positive_interactions++;
    }
}
