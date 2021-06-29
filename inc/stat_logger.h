#ifndef STATLOGGER_H
#define STATLOGGER_H

#include <stdio.h>

#include "synch_ring.h"

typedef struct logger_args logger_args;

logger_args* largs_new(synch_ring* sr_logs, 
                        FILE* outstream);
                            
void largs_delete(logger_args* aa);

void* statt_logger(void* arg);

#endif
