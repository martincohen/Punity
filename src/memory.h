#ifndef MEMORY_H
#define MEMORY_H

typedef struct Memory
{
    u8 *ptr;
    memi count;
    memi capacity;
}
Memory;

void memory_realloc(Memory *buffer, memi capacity);
void *_memory_push(Memory *buffer, memi count, memi capacity);

#define memory_push(buffer, type, count, capacity) \
    ((type *)_memory_push(buffer, count * sizeof(type), capacity * sizeof(type)))

#endif // MEMORY_H
