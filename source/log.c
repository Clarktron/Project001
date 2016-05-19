#include "log.h"

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

FILE *_file = NULL;
const char *_filename = "output.log";
const char *_mode = "w";
uint8_t _setup = 0;

int32_t log_setup()
{
	if (_setup != 0)
	{
		return _setup;
	}
	if (_file == NULL)
	{
		errno_t err = fopen_s(&_file, _filename, _mode);
		if (err)
		{
			printf("log: %i\n", err);
			return -2;
		}
		if (_file == NULL)
		{
			return -3;
		}
		_setup = 1;
		return 0;
	}
	return -1;
}

int32_t log_teardown()
{
	if (_setup != 1)
	{
		return 1;
	}
	if (_file != NULL)
	{
		fclose(_file);
		_setup = 0;
		return 0;
	}
	return -1;
}

int32_t _log_output(const char *format, ...)
{
	int32_t result;
	va_list args;
	time_t t = time(NULL);
	char *str = ctime(&t);
	str[24] = '\0';
	va_start(args, format);
	fprintf_s(stdout, "[%s] ", str);
	result = vfprintf_s(stdout, format, args);
	if (_file != NULL)
	{
		fprintf_s(_file, "[%s] ", str);
		result = vfprintf_s(_file, format, args);
	}
	else
	{
		printf("log: file not open\n");
	}
	va_end(args);
	return result;
}