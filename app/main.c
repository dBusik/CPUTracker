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
#include "stat_watchdog.h"

#define LOGFILE "../mylog.txt"
#define MAIN_THREADS_NUM 3U

static thread_stoppers* stop_controller;

static void term_all_threads(int signum) {
    if(signum == SIGTERM) {
        tstop_stop_threads(stop_controller);
    }
}

int main(void) {
    thread_checkers* work_controller = tcheck_new();
    stop_controller = tstop_new();

    struct sigaction action = { 0, };
    action.sa_handler = term_all_threads;
    sigaction(SIGTERM, &action, NULL);

    synch_ring* sr_reader_analyzer = sring_new(20);
    synch_ring* sr_analyzer_printer = sring_new(20);
    synch_ring* sr_logger = sring_new(30);

    pthread_t reader, analyzer, printer;
    pthread_t logger = { 0, };
    pthread_t watchdog = { 0, };

    reader_args* ra = rargs_new(sr_reader_analyzer, 
                                sr_logger, 
                                stop_controller,
                                work_controller);
   
    analyzer_args* aa = aargs_new(sr_reader_analyzer, 
                                    sr_analyzer_printer, 
                                    sr_logger,
                                    stop_controller,
                                    work_controller);

    printer_args* pa = pargs_new(sr_analyzer_printer, 
                                sr_logger,
                                stop_controller,
                                work_controller);

    pthread_create(&reader, NULL, statt_reader, (void*)&ra);
    pthread_create(&analyzer, NULL, statt_analyzer, (void*)&aa);
    pthread_create(&printer, NULL, statt_printer, (void*)&pa);
    
    /* Logger creation */
    FILE* log_file = fopen(LOGFILE, "w");
    logger_args* la = 0;
    if (log_file) {
        la = largs_new(sr_logger, log_file);
    } else {
        la = largs_new(sr_logger, stdout);
    }
    pthread_create(&logger, NULL, statt_logger, (void*)&la);

    /* Watchdog creation */
    pthread_t main_threads[MAIN_THREADS_NUM] = { reader, analyzer, printer };
    
    watchdog_args* wa = wargs_new(MAIN_THREADS_NUM,
                                        main_threads,
                                        work_controller);

    pthread_create(&watchdog, NULL, statt_watchdog, (void*)&wa);

    /* Main threads termination */    
    pthread_join(printer, NULL);
    pthread_join(analyzer, NULL);
    pthread_join(reader, NULL);

    /* Logger and watchdog termination */    
    pthread_cancel(watchdog);
    pthread_cancel(logger);
    pthread_join(watchdog, NULL);
    pthread_join(logger, NULL);

    if(log_file) fclose(log_file);
    sring_delete(sr_reader_analyzer);
    sring_delete(sr_analyzer_printer);
    sring_delete(sr_logger);

    rargs_delete(ra);
    aargs_delete(aa);
    pargs_delete(pa);
    largs_delete(la);
    wargs_delete(wa);

    tstop_delete(stop_controller);
    tcheck_delete(work_controller);
}
