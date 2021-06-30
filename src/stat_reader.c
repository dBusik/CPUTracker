#include <stdlib.h> 
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <stdatomic.h>

#include "stat_reader.h"
#include "synch_ring.h"
#include "stat_utils.h"
#include "flow_control.h"

#define STAT_LINE_LEN 256U
#define STATFILE "/proc/stat"
#define READER_SLEEP_TIME 1U

/* 
    This small structure is initialized by pointers 
    to its elements, which means that it becomes dangerous 
    if either of them gets free'd by other part of the program.

    USED ONLY AS TEMPORARY HOLDER FOR ARGUMENTS OF READER THREAD  
*/
struct reader_args {
    synch_ring* sr_for_analyzer;
    synch_ring* sr_for_logger;
    thread_stoppers* stop_vars;
    thread_checkers* check_vars;
};

reader_args* rargs_new(synch_ring* sr_for_analyzer, 
                        synch_ring* sr_for_logger,
                        thread_stoppers* stop_vars,
                        thread_checkers* check_vars) 
{
    reader_args* new_ra = 0;
    if (sr_for_analyzer && sr_for_logger) {
        new_ra = malloc(sizeof(*new_ra));
        if (new_ra) {
            *new_ra = (reader_args) {
                .sr_for_analyzer = sr_for_analyzer,
                .sr_for_logger = sr_for_logger,
                .stop_vars = stop_vars,
                .check_vars = check_vars,
            };
        }
    }
    return new_ra;
}

/* Does not affect the buffers inside */
void rargs_delete(reader_args* ra) {
    free(ra);
}

/* 
    Returns number of lines read in stat file 
    (which is the number_of_cores + 1).
    buf_len stores the number of symbols read;
*/
static size_t reader_getstat(char** restrict buffer, 
                                size_t* restrict buf_len,
                                FILE* restrict stat_f) 
{
    size_t result = 0;
    if (stat_f && buffer) {
        size_t needed_len = 0;
        char chunk[STAT_LINE_LEN]; 
        
        /* Read file once to determine the buffer length needed for cpu data */
        while (fgets(chunk, sizeof(chunk), stat_f)) {
            if (util_str_begins_with(chunk, "cpu")) {
                needed_len += strlen(chunk);
            } else {
                break;
            }
        }
        fseek(stat_f, 0, SEEK_SET);

        /* Read cpu data into single buffer */
        char* new_buf = realloc(*buffer, needed_len + 1);
        if (new_buf) {
            size_t cpu_counter = 0;
            *buffer = new_buf;
            *buf_len = needed_len;
            size_t used_len = 0;
            while (fgets(chunk, sizeof(chunk), stat_f)) {
                if (util_str_begins_with(chunk, "cpu")) {
                    memcpy(*buffer + used_len, chunk, strlen(chunk));
                    used_len += strlen(chunk);
                    cpu_counter ++;
                } else {
                    break;
                }
            }
            (*buffer)[used_len] = '\0';
            result = cpu_counter;
        }
    } else {
        perror("Read: invalid file or buffer pointer.\n");
    }
    return result;
}


static void reader_buffer_cleanup(void* arg) {
    if (arg) {
        char** buffer_to_clean = (char**) arg;
        free(*buffer_to_clean);
    }
}

static void reader_file_cleanup(void* arg) {
    if (arg) {
        FILE* file_to_clean = (FILE*) arg;
        fclose(file_to_clean);
    }
}

void* statt_reader(void* arg) {
    FILE* stat_f = 0; 
    /* Holders for number of symbols in /proc/stat and its data */
    size_t file_len = 0;
    char* reader_file_buf = 0;

    pthread_cleanup_push(reader_buffer_cleanup, (void*) &reader_file_buf)
    pthread_cleanup_push(reader_file_cleanup, (void*) stat_f)
    
    reader_args* r_args = *(reader_args**)arg;

    sig_atomic_t volatile* done = tstop_get_reader(r_args->stop_vars);

    synch_ring* sr_for_analyzer = r_args->sr_for_analyzer;
    synch_ring* sr_for_logger = r_args->sr_for_logger;

    SRING_APPEND_STR(sr_for_logger, "Reader thread initialized, entering main loop.");
    
    /* 
        Initial read to match analyzers initial pop from synch_ring 
        (8 lines of duplicated code actually)
    */
    tcheck_reader_activate(r_args->check_vars);
    stat_f = fopen(STATFILE, "r");
    if (reader_getstat(&reader_file_buf, &file_len, stat_f)) {
        SRING_APPEND_STR(sr_for_analyzer, reader_file_buf);
    }
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    fclose(stat_f);
    stat_f = 0;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    while(!(*done)) {
        tcheck_reader_activate(r_args->check_vars);
        sleep(READER_SLEEP_TIME);
        
        stat_f = fopen(STATFILE, "r");
        
        if (stat_f && reader_getstat(&reader_file_buf, &file_len, stat_f)) {
            
            SRING_APPEND_STR(sr_for_analyzer, reader_file_buf);

        }

        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        fclose(stat_f);
        stat_f = 0;
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    }
    
    SRING_APPEND_STR(sr_for_logger, "Reader thread done, already after main loop.");

    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);
    
    return NULL;
}


