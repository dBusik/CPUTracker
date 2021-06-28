#ifndef STATREADER_H
#define STATREADER_H

#include "synch_ring.h"
#include <stddef.h>

typedef struct reader_args reader_args;

reader_args* rargs_new(synch_ring* sr_for_analyzer, 
                        synch_ring* sr_for_logger);

void rargs_delete(reader_args* ra);

void* statt_reader(void* arg);

#endif
