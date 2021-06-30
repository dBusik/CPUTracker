#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "stat_watchdog.h"
#include "flow_control.h"

#define WATCHDOG_SLEEP_TIME 2U

struct watchdog_args {
    thread_checkers* check_vars;
    size_t num_of_threads;
    pthread_t working_threads[];
};


watchdog_args* wargs_new(size_t thread_num, 
                            pthread_t threads[],
                            thread_checkers* check_vars)
{
    watchdog_args* new_wa = 0;
    if (threads && thread_num && check_vars) {
        new_wa = malloc(sizeof(*new_wa) + sizeof(pthread_t) * thread_num);
        if (new_wa) {
            new_wa->check_vars = check_vars;
            new_wa->num_of_threads = thread_num;
            memcpy(new_wa->working_threads, threads, thread_num *  sizeof(pthread_t));
        }
    }
    return new_wa;
}

/* 
    Affectes only members of structure allocated in structure initializer.
*/
void wargs_delete(watchdog_args* wa) {
    free(wa);
}

void* statt_watchdog(void* arg) {
    watchdog_args* w_args = *(watchdog_args**)arg;
    
    size_t threads_num =  w_args->num_of_threads;
    pthread_t* working_threads = w_args->working_threads;
    
    thread_checkers* check_vars = w_args->check_vars;
    
    while(true) {
        sleep(WATCHDOG_SLEEP_TIME);

        if (tcheck_perform_check(check_vars)) {
            tcheck_reset_checks(check_vars);
        } else {
            break;
        }
    }

    /* 
        If one of main threads was not alive the loop was left 
        and watchdog cancels the threads.
        In case of normal ending of the program this code is not reachable,
        because watchdog gets cancelled.
    */
    for (size_t i = 0; i < threads_num; ++i) {
        if (working_threads[i]) {
            pthread_cancel(working_threads[i]);
        }
    }

    return NULL;
}
