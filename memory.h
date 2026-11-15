#ifndef PESTINHA_MEMORY_H
#define PESTINHA_MEMORY_H

#include "runtime_types.h"

void memory_init(EpisodeMemory *m);
void memory_record(EpisodeMemory *m, const char *type, double now_t);

#endif /* PESTINHA_MEMORY_H */
