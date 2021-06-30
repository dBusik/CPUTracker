#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <stdatomic.h>

#include "stat_analyzer.h"
#include "synch_ring.h"
#include "stat_utils.h"
#include "flow_control.h"

/* Base to which numbers from /proc/stat are converted */
#define DATA_NUM_BASE 10

/* 
    This small structure is initialized by pointers 
    to its elements, which means that it becomes dangerous 
    if either of them gets free'd by other part of the program.

    USED ONLY AS TEMPORARY HOLDER FOR ARGUMENTS OF ANALYZER THREAD  
*/
struct analyzer_args {
    synch_ring* sr_from_reader;
    synch_ring* sr_for_printer;
    synch_ring* sr_for_logger;
    thread_stoppers* stop_vars;
    thread_checkers* check_vars;
};

analyzer_args* aargs_new(synch_ring* sr_from_reader, 
                            synch_ring* sr_for_printer,
                            synch_ring* sr_for_logger,
                            thread_stoppers* stop_vars,
                            thread_checkers* check_vars)
{
    analyzer_args* new_aa = 0;
    if (sr_from_reader && sr_for_printer && sr_for_logger) {
        new_aa = malloc(sizeof(*new_aa));
        if (new_aa) {
            *new_aa = (analyzer_args) {
                .sr_from_reader = sr_from_reader,
                .sr_for_printer = sr_for_printer,
                .sr_for_logger = sr_for_logger,
                .stop_vars = stop_vars,
                .check_vars = check_vars,
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
                
                sprintf(small_buff, "%-6.2lf%-1s ", cpu_percentage, "%");
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

static void analyzer_buffer_cleanup(void* arg) {
    if (arg) {
        char** buffer_to_clean = (char**) arg;
        free(*buffer_to_clean);
    }
}

void* statt_analyzer(void* arg) {
    /* Holders for number of symbols in /proc/stat and its data */
    char* old_data_buf = 0;
    char* new_data_buf = 0;
    char* result_data = 0;
    char* temp_buf = 0;
    
    pthread_cleanup_push(analyzer_buffer_cleanup, (void*) &old_data_buf)
    pthread_cleanup_push(analyzer_buffer_cleanup, (void*) &new_data_buf)
    pthread_cleanup_push(analyzer_buffer_cleanup, (void*) &result_data)
    pthread_cleanup_push(analyzer_buffer_cleanup, (void*) &temp_buf)

    analyzer_args* a_args = *(analyzer_args**)arg;
    
    sig_atomic_t volatile* done = tstop_get_analyzer(a_args->stop_vars);
    tcheck_analyzer_activate(a_args->check_vars);
    
    synch_ring* sr_from_reader = a_args->sr_from_reader;
    synch_ring* sr_for_printer = a_args->sr_for_printer;
    synch_ring* sr_for_logger = a_args->sr_for_logger;


    SRING_APPEND_STR(sr_for_logger, "Analyzer thread initialized, doing initial read.");

    /* 
        Initial data read to be able to count CPU usage 
        after first iteration of infinite loop 
    */
    while(!old_data_buf) {
        tcheck_analyzer_activate(a_args->check_vars);
        SRING_POP_STR(sr_from_reader, old_data_buf);
    }

    
    SRING_APPEND_STR(sr_for_logger, "Analyzer thread done first read, entering main loop.");

    while(!(*done)) {
        tcheck_analyzer_activate(a_args->check_vars);
        SRING_POP_STR(sr_from_reader, new_data_buf);

        if (new_data_buf) {
            /* 
                Remember read data becuase string 
                will be changed during parsing 
            */
            temp_buf = strdup(new_data_buf);

            result_data = analyzer_calc(old_data_buf, new_data_buf);
            
            free(new_data_buf);
            new_data_buf = 0;
            
            free(old_data_buf);
            old_data_buf = strdup(temp_buf);
            free(temp_buf);
            temp_buf = 0;

            /* Push result data to buffer from which printer reads */
            SRING_APPEND_STR(sr_for_printer, result_data);

            free(result_data);
            result_data = 0;
        }
    }

    SRING_APPEND_STR(sr_for_logger, "Analyzer thread done, already after main loop.");

    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);

    return NULL;
}

