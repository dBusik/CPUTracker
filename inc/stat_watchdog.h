#ifndef STATWATCHDOG_H
#define STATWATCHDOG_H

#include "flow_control.h"

typedef struct watchdog_args watchdog_args;

watchdog_args* wargs_new(size_t thread_num, 
                            pthread_t threads[],
                            thread_checkers* check_vars);
                            
void wargs_delete(watchdog_args* ra);

void* statt_watchdog(void* arg);

#endif
