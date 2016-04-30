#include "game.h"
#include "input.h"
#include "render.h"
#include "system.h"
#include "log.h"

void game_loop()
{
	//TEXTURE font_test;
	//render_load_text(&font_test, "test", 0, 0x00, 0x00, 0x00, 0x00);
	//int32_t i = 0;
	while (input_poll() == 0)
	{
		render_begin_frame();
		//render_draw_texture(font_test, 100, i++);
		render_end_frame();
	}
}