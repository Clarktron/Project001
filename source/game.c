#include "game.h"
#include "input.h"
#include "render.h"
#include "system.h"
#include "log.h"
#include "menu.h"

#include <stdlib.h>
#include <string.h>

void game_loop()
{
	while (input_poll() == 0)
	{
		render_begin_frame();
		render_end_frame();
	}
}