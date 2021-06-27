#ifndef CPUUTILS_H
#define CPUUTILS_H

#include <stddef.h>

size_t util_read_statfile(char** restrict buffer, size_t* restrict buf_len);
char* util_analyze_stat(char* restrict old_data, char* restrict new_data, size_t n_lines);

#endif
