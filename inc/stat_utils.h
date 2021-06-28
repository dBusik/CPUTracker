#ifndef STATUTILS_H
#define STATUTILS_H

#include <stddef.h>

size_t util_read_statfile(char** restrict buffer, size_t* restrict buf_len);
char* util_analyze_stat(char* restrict old_data, char* restrict new_data);


#endif
