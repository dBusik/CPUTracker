#include "stat_utils.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

char** util_str_split(char* restrict data, const char delimiter, 
                                size_t* restrict n_tokens) {
    if (!data && !n_tokens)
        return 0;

    if (!*n_tokens) {
        char* check = data;
        /* Is this needed (?) */
        *n_tokens = 0;
        while (*check) {
            if (*check == delimiter) {
                ++(*n_tokens);
            }
            ++check;
        }
        if (!*n_tokens) {
            *n_tokens = 1;
        }
    }
    
    char** result = malloc(sizeof(*result) * (*n_tokens));
    if (!result)
        return 0;

    size_t index = 0;
    char delim[2] = { delimiter, '\0' };
    char* token = strtok(data, delim);
    while (token && index < *n_tokens) {
        result[index] = strdup(token);
        token  = strtok(0, delim);
        ++index;
    }
    /* Needed when index < n_tokens */
    *n_tokens = index;

    return result;
}

char* util_str_concat(char const* restrict str1, char const* restrict str2) {
    const size_t s1_len = strlen(str1);
    const size_t s2_len = strlen(str2);

    char* res = malloc(s1_len + s2_len + 1);
    if (!res)
        return 0;
    
    memcpy(res, str1, s1_len);
    memcpy(res + s1_len, str2, s2_len);
    res[s1_len + s2_len] = '\0';
    
    return res;
}

size_t* util_strnums(size_t len, const char line[restrict],
                        size_t* restrict el_num, const int base) 
{
    size_t* result = 0;
    size_t n = 0;
    if (memchr(line, 0, len)) {
        /* 
            In a string consisting only of numbers, max half + 1 of symbols 
            will represent a new number for array. 
            (When string looks like "3 3 3 3 3 .."")
        */
        
        result = malloc(sizeof(*result) * (1 + (len/2)));
        
        for (char* next = 0; line[0]; line = next) {
            result[n] = strtoull(line, &next, base);
            if (line == next) break;
            ++n;
        }

        /* Supposes that shrinking realloc will always succeed. */
        size_t new_len = n ? n : 1;
        result = realloc(result, sizeof(*result) * new_len);
    }
    if (el_num) *el_num = n;
    return result;
}
