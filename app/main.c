#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "synch_ring.h"
#include "stat_reader.h"
#include "stat_analyzer.h"
#include "stat_printer.h"
#include "stat_logger.h"
#include "flow_control.h"

#define LOGFILE "../mylog.txt"

static thread_flow* flow_controller;

static void term_all_threads(int signum) {
    if(signum == SIGTERM) {
        tflow_stop_threads(flow_controller);
    }
}

int main(void) {
    flow_controller = tflow_new();
    
    struct sigaction action = { 0, };
    action.sa_handler = term_all_threads;
    sigaction(SIGTERM, &action, NULL);

    synch_ring* sr_reader_analyzer = sring_new(20);
    synch_ring* sr_analyzer_printer = sring_new(20);
    synch_ring* sr_logger = sring_new(30);

    pthread_t reader, analyzer, printer;
    pthread_t logger = { 0, };

    reader_args* ra = rargs_new(sr_reader_analyzer, 
                                sr_logger, 
                                flow_controller);
   
    analyzer_args* aa = aargs_new(sr_reader_analyzer, 
                                    sr_analyzer_printer, 
                                    sr_logger,
                                    flow_controller);

    printer_args* pa = pargs_new(sr_analyzer_printer, 
                                sr_logger,
                                flow_controller);


    pthread_create(&reader, NULL, statt_reader, (void*)&ra);
    pthread_create(&analyzer, NULL, statt_analyzer, (void*)&aa);
    pthread_create(&printer, NULL, statt_printer, (void*)&pa);
    
    FILE* log_file = fopen(LOGFILE, "w");
    logger_args* la = 0;
    if (log_file) {
        la = largs_new(sr_logger, log_file);
    } else {
        la = largs_new(sr_logger, stdout);
    }
    pthread_create(&logger, NULL, statt_logger, (void*)&la);

    pthread_join(printer, NULL);
    pthread_join(analyzer, NULL);
    pthread_join(reader, NULL);

    pthread_cancel(logger);

    if(log_file) fclose(log_file);
    sring_delete(sr_reader_analyzer);
    sring_delete(sr_analyzer_printer);
    sring_delete(sr_logger);

    rargs_delete(ra);
    aargs_delete(aa);
    pargs_delete(pa);
    largs_delete(la);

    tflow_delete(flow_controller);
}
