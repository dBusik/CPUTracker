#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "synch_ring.h"
#include "stat_reader.h"
#include "stat_analyzer.h"

int main(void) {
    synch_ring* sr_reader_analyzer = sring_new(20);
    synch_ring* sr_analyzer_printer = sring_new(20);
    synch_ring* sr_logger = sring_new(20);

    pthread_t reader, analyzer, printer;
    pthread_t logger;
    (void) logger;
    (void) printer;
    
    reader_args* ra = rargs_new(sr_reader_analyzer, sr_logger);
   
    analyzer_args* aa = aargs_new(sr_reader_analyzer, 
                                    sr_analyzer_printer, 
                                    sr_logger);

    pthread_create(&reader, NULL, statt_reader, (void*)&ra);
    pthread_create(&analyzer, NULL, statt_analyzer, (void*)&aa);

    pthread_join(reader, NULL);
    pthread_join(analyzer, NULL);
}
