#ifndef SYNCHRING_H
#define SYNCHRING_H

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#define SRING_APPEND_STR(sr_buf, data)          \
do {                                            \
    sring_mutex_lock(sr_buf);                   \
    if (sring_is_full(sr_buf)) {                \
        sring_wait_for_consumer(sr_buf);        \
    }                                           \
    sring_append(sr_buf, data, strlen(data));   \
    sring_call_consumer(sr_buf);                \
    sring_mutex_unlock(sr_buf);                 \
} while(false)

#define SRING_POP_STR(sr_buf, target)   \
do {                                    \
    sring_mutex_lock(sr_buf);           \
    if (sring_is_empty(sr_buf)) {       \
        sring_wait_for_producer(sr_buf);\
    }                                   \
    target = sring_pop_front(sr_buf);   \
    sring_call_producer(sr_buf);        \
    sring_mutex_unlock(sr_buf);         \
} while(false)

typedef struct synch_ring synch_ring;

synch_ring* sring_new(size_t max_len);
void sring_clear(synch_ring* sr);
void sring_delete(synch_ring* sr);

bool sring_is_empty(const synch_ring* sr);
bool sring_is_full(const synch_ring* sr);

bool sring_append(synch_ring* restrict sr, char* restrict new_elem, 
                  const size_t elem_len);
char* sring_pop_front(synch_ring* sr);

void sring_print(const synch_ring* sr, char const delim, 
                    FILE* outstream);

void sring_mutex_lock(synch_ring* sr);
void sring_mutex_unlock(synch_ring* sr);
void sring_call_producer(synch_ring* sr);
void sring_call_consumer(synch_ring* sr);
void sring_wait_for_producer(synch_ring* sr);
void sring_wait_for_consumer(synch_ring* sr);

#endif
