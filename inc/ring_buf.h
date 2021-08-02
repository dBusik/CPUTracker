#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct ring_buf ring_buf;

ring_buf* ring_new(const size_t max_len);
void ring_clear(ring_buf* const rb);
void ring_delete(ring_buf* const rb);

size_t ring_length(const ring_buf* const rb);
size_t ring_maxlen(const ring_buf* const rb);
bool ring_is_full(const ring_buf* const rb);

bool ring_append(ring_buf* const rb, const char* const new_elem, const size_t elem_len);
char* ring_pop_front(ring_buf* const rb);

void ring_print(ring_buf const* rb, char const delim, 
                  FILE* outstream);


#endif
