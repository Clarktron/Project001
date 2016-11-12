#include "system.h"
#include "render.h"
#include "log.h"

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <Windows.h>

uint64_t _start_time = 0;

void system_setup()
{
	_start_time = system_time();
	srand((uint32_t)_start_time);
	log_output("system: Setup complete\n");
}

void system_teardown()
{
	log_output("system: Teardown complete\n");
}

int32_t system_rand()
{
	return rand();
}

void system_sleep(uint32_t milliseconds)
{
	Sleep(milliseconds);
}

uint64_t system_time()
{
	SYSTEMTIME st;
	FILETIME ft;
	uint64_t sys_time = 0;

	GetSystemTime(&st);
	SystemTimeToFileTime(&st, &ft);

	sys_time = ((uint64_t)ft.dwLowDateTime);
	sys_time += ((uint64_t)ft.dwHighDateTime) << 32;

	return sys_time - _start_time;
}