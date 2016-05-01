#include "log.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

FILE *_file = NULL;
const char *_filename = "output.log";
const char *_mode = "w";

int32_t log_setup()
{
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
		return 0;
	}
	return -1;
}

int32_t log_teardown()
{
	if (_file != NULL)
	{
		fclose(_file);
		return 0;
	}
	return -1;
}

int32_t log_output(const char *format, ...)
{
	int32_t result;
	va_list args;
	va_start(args, format);
#ifdef PLATFORM_DEBUG
	result = vfprintf_s(stdout, format, args);
#else
	if (_file != NULL)
	{
		result = vfprintf_s(_file, format, args);
	}
	else
	{
		if (_file != NULL)
		{
			result = vfprintf_s(_file, format, args);
		}
		else
		{
			printf("log: file not open\n");
		}
	}
#endif
	va_end(args);

	return result;
}