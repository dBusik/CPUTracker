#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stddef.h>
#include <stdio.h>

typedef struct ring_buf ring_buf;

ring_buf* ring_init(ring_buf* rb, size_t max_len);
void ring_clear(ring_buf* rb);

ring_buf* ring_new(size_t max_len);
void ring_delete(ring_buf* rb);

/* Current length of the buffer */
size_t ring_length(ring_buf* rb);

/* Max length of the buffer */
size_t ring_maxlen(ring_buf* rb);

ring_buf* ring_append(ring_buf* rb, char* restrict new_elem, size_t const elem_len);
char* ring_pop_front(ring_buf* rb);

void ring_print(ring_buf* rb, char delim, FILE* fp);


#endif
