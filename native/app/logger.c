#include <stdio.h>

void _logger(const char *msg, const char *level, int line_num, const char *filename) {
    fprintf(stderr, "[%s:%s:%i] %s\n", level, filename, line_num, msg);
    fflush(stderr);
}
