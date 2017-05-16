#ifndef H_BB_COMMON_H
#define H_BB_COMMON_H

#include <ctype.h>
#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define BB_ASSERT(check, ...) \
        do { \
          if (!(check)) { \
            fprintf(stderr, "Assertion `" #check "` failed. " __VA_ARGS__); \
            exit(1); \
          } \
        } while (false)

#define BB_BZERO(ptr) memset(ptr, 0, sizeof(*(ptr)))
#define BB_DIE(...) do { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); exit(1); } while (false)

#define BB_INFO(...) \
        do { \
            printf("\033[1;32m"); \
            bb_print_timestamp(); \
            putc(' ', stdout); \
            printf(__VA_ARGS__);  \
            printf("\033[1;37m\n"); \
        } while(false)

#define BB_STREQ(s1, s2) (0 == strcmp(s1, s2))


/**
 * Functionality
 **/
bool   bb_is_file_or_dir_hidden(char const* name);
bool   bb_is_source_file(char const* name);
void   bb_print_timestamp(void);
size_t bb_trim_string(char*);


/**
 * Implementation
 **/
bool bb_is_file_or_dir_hidden(char const* name)
{
    char const* last_slash = rindex(name, '/');
    return (last_slash ? last_slash[1] : name[0]) == '.';
}

bool bb_is_source_file(char const* name)
{
    char const* suffix = rindex(name, '.');
    if (!suffix) { return false; }
    return BB_STREQ(suffix, ".h")       || BB_STREQ(suffix, ".c")   ||
           BB_STREQ(suffix, ".hh")      || BB_STREQ(suffix, ".cc")  ||
           BB_STREQ(suffix, ".hpp")     || BB_STREQ(suffix, ".cpp") ||
           BB_STREQ(suffix, ".inl");
}

void bb_print_timestamp(void)
{
    struct timeval tv; gettimeofday(&tv, NULL);
    time_t timer = (time_t)tv.tv_sec;
    struct tm* tm_info = localtime(&timer);
    char buf[64]; strftime(buf, sizeof(buf), "%T", tm_info);
    printf("%s.%03u", buf, (unsigned)(tv.tv_usec / 1000));
}

size_t bb_trim_string(char* s)
{
  size_t head      = 0;         while (isspace(s[head]))                       { head += 1; }
  size_t tail      = strlen(s); while ((tail > head) && (isspace(s[tail-1])))  { tail -= 1; }
  size_t const len = (tail - head);
  if (head != 0) { memmove(s, s + head, len); }
  s[len] = '\0';
  return len;
}

#endif

