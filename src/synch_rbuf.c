#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "ring_buf.h"
#include "synch_ring.h"

struct synch_ring {
   ring_buf* buffer;
   pthread_mutex_t mutex;
   pthread_cond_t append_possible;
   pthread_cond_t pop_possible;
};

synch_ring* sring_new(size_t max_len) {
   synch_ring* new_r = 0;
   if (max_len) {
      new_r = malloc(sizeof(synch_ring));
      if (new_r) {
         *new_r = (synch_ring) {
            .buffer = ring_new(max_len),
            .mutex = PTHREAD_MUTEX_INITIALIZER,
            .append_possible = PTHREAD_COND_INITIALIZER,
            .pop_possible = PTHREAD_COND_INITIALIZER,
         };
         if (!new_r->buffer) {
            free(new_r);
            new_r = 0;
         }
      }

   }
   return new_r;
}

void sring_clear(synch_ring* sr) {
   if (sr) {
      ring_clear(sr->buffer);
   }
}

void sring_delete(synch_ring* sr) {
   if (sr) {
      pthread_mutex_destroy(&sr->mutex);
      pthread_cond_destroy(&sr->append_possible);
      pthread_cond_destroy(&sr->pop_possible);

      ring_delete(sr->buffer);
      free(sr);
   }
}

bool sring_is_empty(synch_ring const* sr) {
   return sr ? !(ring_length(sr->buffer)) : true;
}

bool sring_is_full(synch_ring const* sr) {
   return sr ? ring_is_full(sr->buffer) : false;
}

bool sring_append(synch_ring* sr, char* restrict new_elem, 
                  size_t const elem_len) {
   return sr ? ring_append(sr->buffer, new_elem, elem_len) : false;
}

char* sring_pop_front(synch_ring* sr) {
   return sr ? ring_pop_front(sr->buffer) : 0;
}

void sring_print(synch_ring const* restrict sr, char const delim, 
                  FILE* restrict outstream) 
{
   if (sr) {
      ring_print(sr->buffer, delim, outstream);
   } else {
      perror("Invalid synch_ring pointer");
   }
}

void sring_mutex_lock(synch_ring* sr) {
   pthread_mutex_lock(&sr->mutex);
}

void sring_mutex_unlock(synch_ring* sr) {
   pthread_mutex_unlock(&sr->mutex);
}

void sring_call_producer(synch_ring* sr) {
   pthread_cond_signal(&sr->append_possible);
}

void sring_call_consumer(synch_ring* sr) {
   pthread_cond_signal(&sr->pop_possible);
}

void sring_wait_for_producer(synch_ring* sr) {
   pthread_cond_wait(&sr->pop_possible, &sr->mutex);
}

void sring_wait_for_consumer(synch_ring* sr) {
   pthread_cond_wait(&sr->append_possible, &sr->mutex);
}
