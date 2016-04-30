#include "system.h"

#include "render.h"

#include <stdlib.h>
#include <time.h>
#include <Windows.h>

void system_setup()
{
	srand((uint32_t)time(0));

	render_setup();
}

void system_teardown()
{
	render_teardown();
}

int32_t system_rand()
{
	return rand();
}

void system_sleep(uint32_t milliseconds)
{
	Sleep(milliseconds);
}
