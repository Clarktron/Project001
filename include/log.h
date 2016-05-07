#ifndef LOG_H
#define LOG_H

#include <stdint.h>

int32_t log_setup();
int32_t log_teardown();
int32_t _log_output(const char *format, ...);

#ifdef PLATFORM_DEBUG
#define log_output(fmt, ...) (_log_output(fmt, __VA_ARGS__))
#else
#define log_output(fmt, ...) (0)
#endif

#endif LOG_H