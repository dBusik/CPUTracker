#include "ring_buf.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct ring_buf {
    size_t start;
    size_t len;
    size_t max_len;
    char** arr;
};

/* Maybe clear buffer in the beggining */
ring_buf* ring_init(ring_buf* rb, size_t max_len) {
    if (rb) {
        if (max_len) {
            *rb = (ring_buf) {
                .max_len = max_len,
                .arr = malloc(max_len * sizeof(char*)),
            };
            if (!rb->arr) rb->max_len = 0;
        } else {
            *rb = (ring_buf) { 0, };
        }
    }
    return rb;
}

ring_buf* ring_new(size_t max_len) {
    return ring_init(malloc(sizeof(ring_buf)), max_len);
}

void ring_clear(ring_buf* rb) {
    if (rb) {
        size_t i = rb->start;
        size_t counter = 0;
        while (counter < rb->len) {
            printf("ring_delete: Cleaned %zu bytes. Pos %zu, pop result is \"%s\"\n", strlen(rb->arr[i]) + 1, i, rb->arr[i]);
            free(rb->arr[i]);
            ++i;
            i %= rb->max_len;
            counter ++;
        }
        free(rb->arr);
        ring_init(rb, 0);
    }
}

void ring_delete(ring_buf* rb) {
    ring_clear(rb);
    free(rb);
}

size_t ring_length(ring_buf* rb) {
    return rb ? rb->len : 0;
}

size_t ring_maxlen(ring_buf* rb) {
    return rb ? rb->max_len : 0;
}

/* Next two functions dont perfrom checks because they are for inner usage */
static size_t ring_getpos(ring_buf* rb, size_t pos) {
    pos += rb->start;
    pos %= rb->max_len;
    return pos;
}

/* TODO: check this function (might return not what i want) */
static char** ring_getelem(ring_buf* rb, size_t pos) {
    char** ret = 0;
    if (pos < rb->max_len) {
        size_t real_pos = ring_getpos(rb, pos);
        ret = &rb->arr[real_pos];
    }
    return ret;
}

/* Maybe add a check for null termination of a string = 
    add a function or a loop that just iterates over elem_len and checks if \0 is there. */
ring_buf* ring_append(ring_buf* rb, char* restrict new_elem, size_t const elem_len) {
    if (rb) {
        size_t end_pos = ring_getpos(rb, rb->len);
        printf("mallocing %zu bytes for new element\n", sizeof(**rb->arr) * elem_len + 1);
        rb->arr[end_pos] = malloc(sizeof(**(rb->arr)) * elem_len + 1);
        if (rb->arr[end_pos]) {
            (rb->len)++;
            strncpy(rb->arr[end_pos], new_elem, elem_len);
            (rb->arr[end_pos])[elem_len] = '\0';
            return rb;
        }
    }
    return 0;
}

char* ring_pop_front(ring_buf* rb) {
    char* res = 0;
    if (rb && rb->len) {
        char** first_elem = ring_getelem(rb, 0);
        /* Maybe an overkill check cause we already now that rb->len is not 0 */
        if(first_elem) {
            res = *first_elem;
            printf("Cleaned %zu bytes. Pop result is \"%s\"\n", strlen(*first_elem) + 1, res);
            *first_elem = 0;
            rb->start++;
            rb->len--;
        }    
        printf("Current start %zu, len %zu\n", rb->start, rb->len);
    }
    return res;
}

void ring_print(ring_buf* rb, char delim, FILE* outstream) {
    if (rb && rb->arr) {
        /* If buffer is empty then print nothing */
        if (rb->len >= 1) {
            for (size_t i = 0; i < rb->len; ++i) {
                fprintf(outstream, "%s%c", *ring_getelem(rb, i), delim);
            }
        }
    } else {
        fputs("Invalid buffer or len is 0", outstream);
    }
}
