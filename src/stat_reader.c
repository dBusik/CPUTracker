#include <stdlib.h> 
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "stat_reader.h"
#include "synch_ring.h"
#include "stat_utils.h"

#define STAT_LINE_LEN 256
#define STATFILE "/proc/stat"

/* 
    This small structure is initialized by pointers 
    to its elements, which means that it becomes dangerous 
    if either of them gets free'd by other part of the program.

    USED ONLY AS TEMPORARY HOLDER FOR ARGUMENTS OF READER THREAD  
*/
struct reader_args {
    synch_ring* sr_for_analyzer;
    synch_ring* sr_for_logger;
};

reader_args* rargs_new(synch_ring* sr_for_analyzer, 
                        synch_ring* sr_for_logger) 
{
    reader_args* new_ra = 0;
    if (sr_for_analyzer && sr_for_logger) {
        new_ra = malloc(sizeof(*new_ra));
        if (new_ra) {
            *new_ra = (reader_args) {
                .sr_for_analyzer = sr_for_analyzer,
                .sr_for_logger = sr_for_logger,
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
    Returns 0 upon unsuccessful read of file; otherwise
    X - number of lines read, being the number_of_cores + 1.
    buf_len stores the number of symbols read;
*/
static size_t reader_getstat(char** restrict buffer, 
                                size_t* restrict buf_len) 
{
    FILE* stat_f = fopen(STATFILE, "r"); 
    size_t result = 0;
    if (stat_f && buffer) {
        size_t needed_len = 0;
        char chunk[STAT_LINE_LEN]; 
        
        /* Read file once to determine the buffer length needed for cpu data */
        while (fgets(chunk, sizeof(chunk), stat_f)) {
            if (str_begins_with(chunk, "cpu")) {
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
                if (str_begins_with(chunk, "cpu")) {
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
        fclose(stat_f);
    } else {
        perror("Read: invalid file or buffer pointer.\n");
    }
    return result;
}


void* statt_reader(void* arg) {
    reader_args* r_args = *(reader_args**)arg;

    synch_ring* sr_for_analyzer = r_args->sr_for_analyzer;
    
    synch_ring* sr_for_logger = r_args->sr_for_logger;

    /* Holders for number of symbols in /proc/stat and its data */
    size_t file_len = 1;
    char* file_buffer = malloc(1);

    SRING_APPEND_STR(sr_for_logger, "Reader thread initialized, entering main loop.");

    while(true) {
        /* create some pid_t variable for logger here */
        
        /* Do something if file was read successfully */
        if (reader_getstat(&file_buffer, &file_len)) {
            
            SRING_APPEND_STR(sr_for_analyzer, file_buffer);

        }
        sleep(1);
    }

    SRING_APPEND_STR(sr_for_logger, "Reader thread done, already after main loop.");

    return NULL;
}


