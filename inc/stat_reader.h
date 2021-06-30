#ifndef STATREADER_H
#define STATREADER_H

#include "synch_ring.h"
#include "flow_control.h"

typedef struct reader_args reader_args;

reader_args* rargs_new(synch_ring* sr_for_analyzer, 
                        synch_ring* sr_for_logger,
                        thread_stoppers* stop_vars,
                        thread_checkers* check_vars);

void rargs_delete(reader_args* ra);

void* statt_reader(void* arg);

#endif
