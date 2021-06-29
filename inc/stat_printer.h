#ifndef STATPRINTER_H
#define STATPRINTER_H

#include "synch_ring.h"
#include "flow_control.h"

typedef struct printer_args printer_args;

printer_args* pargs_new(synch_ring* sr_from_analyzer, 
                        synch_ring* sr_for_logger,
                        thread_flow* flow_vars);
                            
void pargs_delete(printer_args* pa);

void* statt_printer(void* arg);

#endif
