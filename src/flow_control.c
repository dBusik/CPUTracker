#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdio.h>

#include "flow_control.h"

struct thread_stoppers {
    volatile sig_atomic_t reader_done;
    volatile sig_atomic_t analyzer_done;
    volatile sig_atomic_t printer_done;
};

thread_stoppers* tstop_new() {
    thread_stoppers* new_tstop = malloc(sizeof(thread_stoppers));
    if (!new_tstop)
        return 0;

    *new_tstop = (thread_stoppers) {
        .reader_done = 0,
        .analyzer_done = 0,
        .printer_done = 0,
    };
    return new_tstop;
}

void tstop_delete(thread_stoppers* ts) {
    free(ts);
}

void tstop_stop_threads(thread_stoppers* ts) {
    if (!ts)
        return;

    ts->reader_done = 1;
    ts->analyzer_done = 1;
    ts->printer_done = 1;
}

sig_atomic_t volatile* tstop_get_reader(thread_stoppers* ts) {
    return ts ? &ts->reader_done : 0;
}

sig_atomic_t volatile* tstop_get_analyzer(thread_stoppers* ts) {
    return ts ? &ts->analyzer_done : 0;
}

sig_atomic_t volatile* tstop_get_printer(thread_stoppers* ts) {
    return ts ? &ts->printer_done : 0;
}

struct thread_checkers {
    atomic_bool reader_works;
    atomic_bool analyzer_works;
    atomic_bool printer_works;
};

thread_checkers* tcheck_new(void) {
    thread_checkers* new_tcheck = malloc(sizeof(*new_tcheck));
    if (!new_tcheck)
        return 0;

    atomic_init(&new_tcheck->reader_works, false);
    atomic_init(&new_tcheck->analyzer_works, false);
    atomic_init(&new_tcheck->printer_works, false);
    
    return new_tcheck;
}

void tcheck_delete(thread_checkers* tc) {
    free(tc);
}

void tcheck_reader_activate(thread_checkers* tc) {
    if (!tc)
        return;

    atomic_store(&tc->reader_works, true);
}

void tcheck_analyzer_activate(thread_checkers* tc) {
    if (!tc)
        return;

    atomic_store(&tc->analyzer_works, true);
}

void tcheck_printer_activate(thread_checkers* tc) {
    if (!tc)
        return;

    atomic_store(&tc->printer_works, true);
}

bool tcheck_perform_check(thread_checkers* tc) {
    if (!tc)
        return false;

    return atomic_load(&tc->reader_works)
            && atomic_load(&tc->analyzer_works)
            && atomic_load(&tc->printer_works);
}

void tcheck_reset_checks(thread_checkers* tc) {
    if (!tc)
        return;

    atomic_store(&tc->reader_works, false);
    atomic_store(&tc->analyzer_works, false);
    atomic_store(&tc->printer_works, false);
}
