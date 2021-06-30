#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <stdatomic.h>

#include "stat_printer.h"
#include "synch_ring.h"
#include "flow_control.h"

#define CLEAR_STDOUT() printf("\033c")

/* 
    This small structure is initialized by pointers 
    to its elements, which means that it becomes dangerous 
    if either of them gets free'd by other part of the program.

    USED ONLY AS TEMPORARY HOLDER FOR ARGUMENTS OF PRINTER THREAD  
*/
struct printer_args {
    synch_ring* sr_from_analyzer;
    synch_ring* sr_for_logger;
    thread_stoppers* stop_vars;
    thread_checkers* check_vars;
};

printer_args* pargs_new(synch_ring* sr_from_analyzer, 
                        synch_ring* sr_for_logger,
                        thread_stoppers* stop_vars,
                        thread_checkers* check_vars) 
{
    printer_args* new_pa = 0;
    if (sr_for_logger && sr_from_analyzer) {
        new_pa = malloc(sizeof(*new_pa));
        if (new_pa) {
            *new_pa = (printer_args) {
                .sr_from_analyzer = sr_from_analyzer,
                .sr_for_logger = sr_for_logger,
                .stop_vars = stop_vars,
                .check_vars = check_vars,
            };
        }
    }
    return new_pa;
}

/* Does not affect the buffers inside */
void pargs_delete(printer_args* pa) {
    free(pa);
}

static void printer_buffer_cleanup(void* arg) {
    if (arg) {
        char** buffer_to_clean = (char**) arg;
        free(*buffer_to_clean);
    }
}

void* statt_printer(void* arg) {
    char* calc_stats = 0;
    int core_counter = 0;
    size_t data_len = 0;

    pthread_cleanup_push(printer_buffer_cleanup, (void*) &calc_stats)

    printer_args* p_args = *(printer_args**)arg;
    
    sig_atomic_t volatile* done = tstop_get_printer(p_args->stop_vars);
    tcheck_printer_activate(p_args->check_vars);
    
    synch_ring* sr_from_analyzer = p_args->sr_from_analyzer;
    synch_ring* sr_for_logger = p_args->sr_for_logger;
    
    SRING_APPEND_STR(sr_for_logger, "Printer thread initialized, entering main loop.");
    CLEAR_STDOUT();
    while(!(*done)) {
        tcheck_printer_activate(p_args->check_vars);
        SRING_POP_STR(sr_from_analyzer, calc_stats);
        
        printf("%-7s ", "cpu");
        data_len = strlen(calc_stats);
        for (char* i = strchr(calc_stats, '%') + 1; i < calc_stats+data_len; ++i) {
            if (*i == '%') {
                printf("cpu%-4d ", core_counter);
                ++core_counter;
            }
        }
        core_counter = 0;

        printf("\n%s\n", calc_stats);   
        for (size_t i = 0; i < data_len; ++i) {
            printf("-");
        }
        printf("\n");
        CLEAR_STDOUT();

        free(calc_stats);
        calc_stats = 0;
    }

    SRING_APPEND_STR(sr_for_logger, "Printer thread done, already after main loop.");
    
    pthread_cleanup_pop(1);
    
    return NULL;
}
