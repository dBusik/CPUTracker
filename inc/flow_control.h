#ifndef STATFLOW_H
#define STATFLOW_H

typedef struct thread_flow thread_flow;

thread_flow* tflow_new(void);
void tflow_delete(thread_flow* tf);

sig_atomic_t volatile* tflow_get_reader(thread_flow* tf);
sig_atomic_t volatile* tflow_get_analyzer(thread_flow* tf);
sig_atomic_t volatile* tflow_get_printer(thread_flow* tf);

void tflow_stop_threads(thread_flow* tf);

#endif
