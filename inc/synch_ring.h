#ifndef SYNCHRING_H
#define SYNCHRING_H

#include <stddef.h>
#include <stdio.h>

typedef struct synch_ring synch_ring;

synch_ring* sring_new(size_t max_len);
void sring_clear(synch_ring* sr);
void sring_delete(synch_ring* sr);

bool sring_is_empty(synch_ring const* sr);
bool sring_is_full(synch_ring const* sr);

bool sring_append(synch_ring* restrict sr, char* restrict new_elem, 
                  size_t const elem_len);
char* sring_pop_front(synch_ring* restrict sr);

void sring_print(synch_ring const* restrict sr, char const delim, 
                    FILE* restrict outstream);

void sring_mutex_lock(synch_ring* sr);
void sring_mutex_unlock(synch_ring* sr);
void sring_call_producer(synch_ring* sr);
void sring_call_consumer(synch_ring* sr);
void sring_wait_for_producer(synch_ring* sr);
void sring_wait_for_consumer(synch_ring* sr);

#endif
