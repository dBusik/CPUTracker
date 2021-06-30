#ifndef STATFLOW_H
#define STATFLOW_H

#include <signal.h>
#include <stdatomic.h>
/* 
    Structure used for stopping threads 'legally' 
    (contains variables controlling threads' loops) 
*/
typedef struct thread_stoppers thread_stoppers;

thread_stoppers* tstop_new(void);
void tstop_delete(thread_stoppers* ts);

sig_atomic_t volatile* tstop_get_reader(thread_stoppers* ts);
sig_atomic_t volatile* tstop_get_analyzer(thread_stoppers* ts);
sig_atomic_t volatile* tstop_get_printer(thread_stoppers* ts);

void tstop_stop_threads(thread_stoppers* ts);

/*
    Structure used by watchdog to check whether threads are
    running.
*/
typedef struct thread_checkers thread_checkers;

thread_checkers* tcheck_new(void);
void tcheck_delete(thread_checkers* tc);

void tcheck_reader_activate(thread_checkers* tc);
void tcheck_analyzer_activate(thread_checkers* tc);
void tcheck_printer_activate(thread_checkers* tc);

bool tcheck_perform_check(thread_checkers* tc);
void tcheck_reset_checks(thread_checkers* tc);

#endif
