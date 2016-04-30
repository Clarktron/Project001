#ifndef LOG_H
#define LOG_H

#include <stdint.h>

int32_t log_setup();
int32_t log_teardown();
int32_t log_output(const char *format, ...);

#endif LOG_H