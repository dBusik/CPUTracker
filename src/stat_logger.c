#include <stdlib.h> 
#include <stdio.h>
#include <pthread.h>

#include "stat_logger.h"
#include "synch_ring.h"

/* 
    This small structure is initialized by pointers 
    to its elements, which means that it becomes dangerous 
    if either of them gets free'd by other part of the program.

    USED ONLY AS TEMPORARY HOLDER FOR ARGUMENTS OF LOGGER THREAD  
*/
struct logger_args {
    synch_ring* sr_logs;
    FILE* outstream;
};

logger_args* largs_new(synch_ring* sr_logs, 
                        FILE* outstream) {
    logger_args* new_ra = 0;
    if (sr_logs && outstream) {
        new_ra = malloc(sizeof(*new_ra));
        if (new_ra) {
            *new_ra = (logger_args) {
                .sr_logs = sr_logs,
                .outstream = outstream,
            };
        }
    }
    return new_ra;
}

/* Does not affect the buffers inside */
void largs_delete(logger_args* ra) {
    free(ra);
}

static void logger_buffer_cleanup(void* arg) {
    if (arg) {
        char** buffer_to_clean = (char**) arg;
        free(*buffer_to_clean);
    }
}

void* statt_logger(void* arg) {
    char* log = 0;
    
    pthread_cleanup_push(logger_buffer_cleanup, (void*) &log)

    logger_args* l_args = *(logger_args**)arg;

    synch_ring* sr_logs = l_args->sr_logs;
    FILE* outstream = l_args->outstream;

    while(true) {
        
        SRING_POP_STR(sr_logs, log);
        
        fprintf(outstream, "%s\n", log);        
        free(log);
        log = 0;
    }

    pthread_cleanup_pop(1);

    return NULL;
}
