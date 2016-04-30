#include "system.h"

#include "render.h"

#include <SDL.h>
#include <stdlib.h>
#include <time.h>

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