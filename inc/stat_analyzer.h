#ifndef STATANALYZER_H
#define STATANALYZER_H

#include "synch_ring.h"
#include "flow_control.h"

typedef struct analyzer_args analyzer_args;

analyzer_args* aargs_new(synch_ring* sr_from_reader, 
                            synch_ring* sr_for_printer,
                            synch_ring* sr_for_logger,
                            thread_flow* flow_vars);
                            
void aargs_delete(analyzer_args* aa);

void* statt_analyzer(void* arg);

#endif
