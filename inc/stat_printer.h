#ifndef STATPRINTER_H
#define STATPRINTER_H

#include "synch_ring.h"

typedef struct printer_args printer_args;

printer_args* pargs_new(synch_ring* sr_from_analyzer, synch_ring* sr_logs);
                            
void pargs_delete(printer_args* pa);

void* statt_printer(void* arg);

#endif
