#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define STAT_LINE_LEN 256

inline
static bool str_begins_with(char const* str, char const* match_str) {
    return strncmp(str, match_str, strlen(match_str)) == 0;
}

/* 
    Returns 0 upon unsuccessful read of file; otherwise
    X - number of lines read, being the number_of_cores + 1 
*/
size_t util_read_statfile(char** restrict buffer, 
                        size_t* restrict buf_len) 
{
    FILE* stat_f = fopen("/proc/stat", "r"); 
    size_t result = 0;
    if (stat_f && buffer) {
        size_t needed_len = 0;
        char chunk[STAT_LINE_LEN + 1]; 
        /* Read file once to determine the buffer length needed for cpu data */
        while (fgets(chunk, sizeof(chunk), stat_f)) {
            if (str_begins_with(chunk, "cpu")) {
                needed_len += strlen(chunk);
            } else {
                break;
            }
        }
        fseek(stat_f, 0, SEEK_SET);

        /* Read cpu data into single buffer */
        char* new_buf = realloc(*buffer, needed_len + 1);
        if (new_buf) {
            size_t cpu_counter = 0;
            *buffer = new_buf;
            *buf_len = needed_len;
            size_t used_len = 0;
            while (fgets(chunk, sizeof(chunk), stat_f)) {
                if (str_begins_with(chunk, "cpu")) {
                    memcpy(*buffer + used_len, chunk, strlen(chunk));
                    used_len += strlen(chunk);
                    cpu_counter ++;
                } else {
                    break;
                }
            }
            (*buffer)[used_len] = '\0';
            result = cpu_counter;
        }
        fclose(stat_f);
    } else {
        perror("Read: invalid file or buffer pointer.\n");
    }
    return result;
}

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
static char** util_str_split(char* restrict data, char const delimiter, 
                                size_t* restrict n_tokens) {
    char** result = 0;
    if (data && n_tokens) {
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
        result = malloc(sizeof(*result) * (*n_tokens));
        if (result) {
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
        }
    }
    return result;
}

static char* str_concat(char const* const str1, char const* const str2) {
    const size_t s1_len = strlen(str1);
    const size_t s2_len = strlen(str2);

    char* res = malloc(s1_len + s2_len + 1);
    if (res) {
        memcpy(res, str1, s1_len);
        memcpy(res + s1_len, str2, s2_len);
        res[s1_len + s2_len] = '\0';
    }
    
    return res;
}

static size_t* util_strnums(size_t len, char const line[],
                                size_t* restrict el_num, int const base) 
{
    size_t* result = 0;
    size_t n = 0;
    if (memchr(line, 0, len)) {
        /* TODO: Magic numbers explanataion */
        
        result = malloc(sizeof(*result) * (1 + (2 * len)/3));
        
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

char* util_analyze_stat(char* restrict old_data, char* restrict new_data, size_t const n_lines) {
    char* calc_cpu_data = strdup("");
    size_t real_lines = n_lines;
    char** old_cpu_data = util_str_split(old_data, '\n', &real_lines);
    char** new_cpu_data = util_str_split(new_data, '\n', &real_lines);

         
    if (old_cpu_data && new_cpu_data && real_lines == n_lines) {
        /*
        printf("Old_line:\n");
        for (size_t i = 0; i < n_lines; ++i) {
            printf("%s\n", old_cpu_data[i]);
        }
        printf("New_line:\n");
        for (size_t i = 0; i < n_lines; ++i) {
            printf("%s\n", new_cpu_data[i]);
        }
        */
        /* Base to which numbers from /proc/stat are converted */
        register int const base = 10;
        
        size_t prev_idle = 0;
        size_t prev_non_idle = 0;
        size_t prev_total = 0;
        
        size_t idle = 0;
        size_t non_idle = 0;
        size_t total = 0;

        size_t total_diff = 0;
        size_t idle_diff = 0;

        double cpu_percentage = 0.0;

        size_t num_count = 0;
        char small_buff[12] = { 0, };

        for (size_t i = 0; i < n_lines; ++i) {
            size_t* old_cpu_line = util_strnums(strlen(old_cpu_data[i]), strchr(old_cpu_data[i], ' '), &num_count, base);
            size_t* new_cpu_line = util_strnums(strlen(new_cpu_data[i]), strchr(new_cpu_data[i], ' '), &num_count, base);
            /*
            printf("2Old_line:\n");
            for (size_t j = 0; j < num_count; ++j) {
                printf("%zu ", old_cpu_line[j]);
            }
            printf("\n");
            
            printf("2New_line:\n");
            for (size_t j = 0; j < num_count; ++j) {
                printf("%zu ", new_cpu_line[j]);
            }
            printf("\n");
            */
            if (old_cpu_line && new_cpu_line) {
                prev_idle = old_cpu_line[3] + old_cpu_line[4];
                prev_non_idle = old_cpu_line[0] + old_cpu_line[1] + old_cpu_line[2] + old_cpu_line[5] + old_cpu_line[6] + old_cpu_line[7];
                prev_total = prev_idle + prev_non_idle;

                idle = new_cpu_line[3] + new_cpu_line[4];
                non_idle = new_cpu_line[0] + new_cpu_line[1] + new_cpu_line[2] + new_cpu_line[5] + new_cpu_line[6] + new_cpu_line[7];
                total = idle + non_idle;
                
                total_diff = total - prev_total;
                idle_diff = idle - prev_idle;
            
                cpu_percentage = (double)(total_diff - idle_diff)/(double)total_diff * 1e2;
                sprintf(small_buff, "%.2lf ", cpu_percentage);
                char* temp = str_concat(calc_cpu_data, small_buff);
                free(calc_cpu_data);
                calc_cpu_data = temp;
            }

            free(old_cpu_line);
            free(new_cpu_line);
        }
        for (size_t i = 0; i < n_lines; ++i) {
            free(old_cpu_data[i]);
            free(new_cpu_data[i]); 
        }
        free(old_cpu_data);
        free(new_cpu_data);
    }

    return calc_cpu_data;
}
