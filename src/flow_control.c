#include <signal.h>
#include <stdlib.h>

#include "flow_control.h"

struct thread_flow {
    volatile sig_atomic_t reader_done;
    volatile sig_atomic_t analyzer_done;
    volatile sig_atomic_t printer_done;
};

thread_flow* tflow_new() {
    thread_flow* new_tflow = malloc(sizeof(thread_flow));
    *new_tflow = (thread_flow) {
        .reader_done = 0,
        .analyzer_done = 0,
        .printer_done = 0,
    };
    return new_tflow;
}

void tflow_delete(thread_flow* tf) {
    free(tf);
}

void tflow_stop_threads(thread_flow* tf) {
    if (tf) {
        tf->reader_done = 1;
        tf->analyzer_done = 1;
        tf->printer_done = 1;
    }
}

sig_atomic_t volatile* tflow_get_reader(thread_flow* tf) {
    return tf ? &tf->reader_done : 0;
}

sig_atomic_t volatile* tflow_get_analyzer(thread_flow* tf) {
    return tf ? &tf->analyzer_done : 0;
}

sig_atomic_t volatile* tflow_get_printer(thread_flow* tf) {
    return tf ? &tf->printer_done : 0;
}
