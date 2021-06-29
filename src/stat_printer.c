#include <stdlib.h> 
#include <stdio.h>
#include <string.h>

#include "stat_printer.h"
#include "synch_ring.h"

/* 
    This small structure is initialized by pointers 
    to its elements, which means that it becomes dangerous 
    if either of them gets free'd by other part of the program.

    USED ONLY AS TEMPORARY HOLDER FOR ARGUMENTS OF PRINTER THREAD  
*/
struct printer_args {
    synch_ring* sr_from_analyzer;
    synch_ring* sr_for_logger;
};

printer_args* pargs_new(synch_ring* sr_from_analyzer, synch_ring* sr_for_logger) 
{
    printer_args* new_pa = 0;
    if (sr_for_logger && sr_from_analyzer) {
        new_pa = malloc(sizeof(*new_pa));
        if (new_pa) {
            *new_pa = (printer_args) {
                .sr_from_analyzer = sr_from_analyzer,
                .sr_for_logger = sr_for_logger,
            };
        }
    }
    return new_pa;
}

/* Does not affect the buffers inside */
void pargs_delete(printer_args* pa) {
    free(pa);
}

void* statt_printer(void* arg) {
    printer_args* p_args = *(printer_args**)arg;

    synch_ring* sr_from_analyzer = p_args->sr_from_analyzer;
    synch_ring* sr_for_logger = p_args->sr_for_logger;

    char* calc_stats = 0;
    int counter = 0;
    size_t len = 0;
    
    SRING_APPEND_STR(sr_for_logger, "Printer thread initialized, entering main loop.");
    
    while(true) {

        SRING_POP_STR(sr_from_analyzer, calc_stats);
        
        printf("%-7s ", "cpu");
        len = strlen(calc_stats);
        for (char* i = strchr(calc_stats, '%') + 1; i < calc_stats+len; ++i) {
            if (*i == '%') {
                printf("cpu%-4d ", counter);
                ++counter;
            }
        }
        counter = 0;

        printf("\n%s\n", calc_stats);   
        for (size_t i = 0; i < len; ++i) {
            printf("-");
        }
        printf("\n");
        
        free(calc_stats);
        calc_stats = 0;
    }

    SRING_APPEND_STR(sr_for_logger, "Printer thread done, already after main loop.");

    return NULL;
}
