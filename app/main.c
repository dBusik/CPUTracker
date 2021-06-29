#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "synch_ring.h"
#include "stat_reader.h"
#include "stat_analyzer.h"
#include "stat_printer.h"
#include "stat_logger.h"

#define LOGFILE "../mylog.txt"

int main(void) {
    synch_ring* sr_reader_analyzer = sring_new(20);
    synch_ring* sr_analyzer_printer = sring_new(20);
    synch_ring* sr_logger = sring_new(30);

    pthread_t reader, analyzer, printer;
    pthread_t logger = { 0, };

    reader_args* ra = rargs_new(sr_reader_analyzer, sr_logger);
   
    analyzer_args* aa = aargs_new(sr_reader_analyzer, 
                                    sr_analyzer_printer, 
                                    sr_logger);

    printer_args* pa = pargs_new(sr_analyzer_printer, sr_logger);


    pthread_create(&reader, NULL, statt_reader, (void*)&ra);
    pthread_create(&analyzer, NULL, statt_analyzer, (void*)&aa);
    pthread_create(&printer, NULL, statt_printer, (void*)&pa);
    
    FILE* log_file = fopen(LOGFILE, "w");
    if (log_file) {
        logger_args* la = largs_new(sr_logger, stdout);
        pthread_create(&logger, NULL, statt_logger, (void*)&la);
    }

    pthread_join(reader, NULL);
    pthread_join(analyzer, NULL);
    pthread_join(printer, NULL);
    pthread_join(logger, NULL);

    fclose(log_file);
    sring_delete(sr_reader_analyzer);
    sring_delete(sr_analyzer_printer);
    sring_delete(sr_logger);

    rargs_delete(ra);
    aargs_delete(aa);
    pargs_delete(pa);
}
