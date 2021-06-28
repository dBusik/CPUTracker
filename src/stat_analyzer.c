#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "stat_analyzer.h"
#include "synch_ring.h"
#include "stat_utils.h"

/* Base to which numbers from /proc/stat are converted */
#define DATA_NUM_BASE 10

/* 
    Small unsafe structure consisting of three synch_ring buffers. 
    Initialized by pointers to buffers, which means that it 
    becomes dangerous if either of the buffers gets 
    removed by other part of the program.

    USED ONLY AS TEMPORARY HOLDER FOR ARGUMENTS OF ANALYZER THREAD  
*/
struct analyzer_args {
    synch_ring* sr_from_reader;
    synch_ring* sr_for_printer;
    synch_ring* sr_for_logger;
};

analyzer_args* aargs_new(synch_ring* sr_from_reader, 
                            synch_ring* sr_for_printer,
                            synch_ring* sr_for_logger)
{
    analyzer_args* new_aa = 0;
    if (sr_from_reader && sr_for_printer && sr_for_logger) {
        new_aa = malloc(sizeof(*new_aa));
        if (new_aa) {
            *new_aa = (analyzer_args) {
                .sr_from_reader = sr_from_reader,
                .sr_for_printer = sr_for_printer,
                .sr_for_logger = sr_for_logger,
            };
        }
    }
    return new_aa;
}

/* Does not affect the buffers inside */
void aargs_delete(analyzer_args* aa) {
    free(aa);
}

/* Modifies argument strings! */
static char* analyzer_calc(char* restrict old_data, char* restrict new_data) {
    char* calc_cpu_data = strdup("");
    size_t lines_num = 0;
    char** old_cpu_data = util_str_split(old_data, '\n', &lines_num);
    char** new_cpu_data = util_str_split(new_data, '\n', &lines_num);

         
    if (old_cpu_data && new_cpu_data) {               
        size_t prev_idle = 0;
        size_t prev_non_idle = 0;
        size_t prev_total = 0;
        
        size_t idle = 0;
        size_t non_idle = 0;
        size_t total = 0;

        size_t total_diff = 0;
        size_t idle_diff = 0;

        double cpu_percentage = 0.0;

        size_t num_count = 0;
        char small_buff[12] = { 0, };

        for (size_t i = 0; i < lines_num; ++i) {
            size_t* old_cpu_line = util_strnums(
                strlen(old_cpu_data[i]), strchr(old_cpu_data[i], ' '), &num_count, DATA_NUM_BASE);
            size_t* new_cpu_line = util_strnums(
                strlen(new_cpu_data[i]), strchr(new_cpu_data[i], ' '), &num_count, DATA_NUM_BASE);
            if (old_cpu_line && new_cpu_line) {
                
                /* Calculations for older read of /proc/stat */
                prev_idle = old_cpu_line[3] + old_cpu_line[4];
                
                prev_non_idle = old_cpu_line[0] + old_cpu_line[1] + old_cpu_line[2] 
                    + old_cpu_line[5] + old_cpu_line[6] + old_cpu_line[7];
                
                prev_total = prev_idle + prev_non_idle;

                /* Calculations for newer read of /proc/stat */
                idle = new_cpu_line[3] + new_cpu_line[4];
                
                non_idle = new_cpu_line[0] + new_cpu_line[1] + new_cpu_line[2] 
                    + new_cpu_line[5] + new_cpu_line[6] + new_cpu_line[7];
                
                total = idle + non_idle;
                
                /* Comparison of two earlier calculations */
                total_diff = total - prev_total;
                idle_diff = idle - prev_idle;
            
                cpu_percentage = (double)(total_diff - idle_diff)/(double)total_diff * 1e2;
                
                sprintf(small_buff, "%.2lf ", cpu_percentage);
                char* mid_result_str = util_str_concat(calc_cpu_data, small_buff);
                free(calc_cpu_data);
                calc_cpu_data = mid_result_str;
            }

            free(old_cpu_line);
            free(new_cpu_line);
        }
        for (size_t i = 0; i < lines_num; ++i) {
            free(old_cpu_data[i]);
            free(new_cpu_data[i]); 
        }
        free(old_cpu_data);
        free(new_cpu_data);
    }

    return calc_cpu_data;
}

void* statt_analyzer(void* arg) {
    analyzer_args* a_args = *(analyzer_args**)arg;
    synch_ring* sr_from_reader = a_args->sr_from_reader;
    synch_ring* sr_for_printer = a_args->sr_for_printer;
    synch_ring* sr_for_logger = a_args->sr_for_logger;

    /* Holders for number of symbols in /proc/stat and its data */
    size_t data_counter = 0;
    size_t n_lines = 0;
    char* old_data_buf = 0;
    char* new_data_buf = 0;
    char* temp_buf = 0;
    char* result_data = 0;

    (void)data_counter;
    (void)n_lines;
    (void)sr_for_printer;
    (void)sr_for_logger;

    while(!old_data_buf) {
        sring_mutex_lock(sr_from_reader);
        if (sring_is_empty(sr_from_reader)) {
            sring_wait_for_producer(sr_from_reader);
        }

        old_data_buf = sring_pop_front(sr_from_reader);

        sring_call_producer(sr_from_reader);
        sring_mutex_unlock(sr_from_reader);
    }

    while(true) {
        /* create some pid_t variable for logger here */

        sring_mutex_lock(sr_from_reader);
        if (sring_is_empty(sr_from_reader)) {
            sring_wait_for_producer(sr_from_reader);
        }

        new_data_buf = sring_pop_front(sr_from_reader);

        sring_call_producer(sr_from_reader);
        sring_mutex_unlock(sr_from_reader);

        if (new_data_buf) {
            temp_buf = strdup(new_data_buf);

            printf("old_data:%s\n", old_data_buf);
            printf("new_data:%s\n", new_data_buf);
            
            result_data = analyzer_calc(old_data_buf, new_data_buf);
            
            free(old_data_buf);
            free(new_data_buf);
            new_data_buf = 0;
            
            old_data_buf = temp_buf;

            printf("results: %s\n", result_data);

            /* Push result data to buffer from which printer reads */
            /*sring_mutex_lock(sr_for_printer);
            if (sring_is_full(sr_for_printer)) {
                sring_wait_for_consumer(sr_for_printer);
            }

            sring_append(sr_for_printer, result_data, strlen(result_data));

            sring_call_consumer(sr_for_printer);
            sring_mutex_unlock(sr_for_printer);*/

            free(result_data);
            result_data = 0;
        }
    }

    return NULL;
}

