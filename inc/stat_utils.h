#ifndef STATUTILS_H
#define STATUTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <string.h>

inline
static bool util_str_begins_with(char const* str, char const* match_str) {
    return strncmp(str, match_str, strlen(match_str)) == 0;
}

char* util_str_concat(char const* restrict str1, char const* restrict str2);

/*
    Split a string consisting of numbers into actual numbers and put 
    them into array. Returns mentioned array of numbers.
*/
size_t* util_strnums(size_t len, char const line[],
                        size_t* restrict el_num, int const base);

/* 
    Split a string into n_tokens tokens. 
    If n_tokens is 0, then function will determine their number 
    and store it to n_tokens parameter.
    If n_tokens is less than the possible number of tokens in string,
    then first n_tokens tokens are returned. 
    If n_tokens exeeds the possible number of tokens, then the possible
    number of tokens is returned and n_tokens is set to that number.
    If delimiter is not present in a string or '\0' is a delimiter, 
    then a 1-element array consisting of one initial string is returned.
    Does not handle consecutive delimiters.
*/
char** util_str_split(char* restrict data, char const delimiter, 
                                size_t* restrict n_tokens);

#endif
