#include "memory.h"

void
memory_realloc(Memory *buffer, memi capacity)
{
    buffer->ptr = realloc(buffer->ptr, capacity);
    buffer->capacity = capacity;
}

void *
_memory_push(Memory *buffer, memi count, memi capacity)
{
    memi c = buffer->count + count;
    if (c > buffer->capacity) {
        assert(capacity >= count);
        memory_realloc(buffer, buffer->capacity + capacity);
    }

    void *ptr = buffer->ptr + buffer->count;
    buffer->count = c;
    return ptr;
}
