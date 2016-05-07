#include "game.h"
#include "input.h"
#include "render.h"
#include "system.h"
#include "log.h"
#include "menu.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void _game_play(GAME_STATE *state);

void game_loop()
{
	GAME_STATE state = STATE_MENU_MAIN;

	while (input_poll() == 0)
	{
		render_begin_frame();
		switch (state)
		{
			case STATE_MENU_MAIN:
			case STATE_MENU_SINGLE_OPTIONS:
			case STATE_MENU_MULTI_OPTIONS:
				menu_display(&state);
				break;
			case STATE_GAME_SINGLE:
				_game_play(&state);
				break;
			case STATE_GAME_MULTI:
				break;
			case STATE_EXIT:
				return;
		}
		render_end_frame();
	}
}

char *game_state_to_string(GAME_STATE state)
{
	switch (state)
	{
		case STATE_MENU_MAIN:
			return "STATE_MENU_MAIN";
		case STATE_MENU_SINGLE_OPTIONS:
			return "STATE_MENU_SINGLE_OPTIONS";
		case STATE_MENU_MULTI_OPTIONS:
			return "STATE_MENU_MULTI_OPTIONS";
		case STATE_GAME_SINGLE:
			return "STATE_GAME_SINGLE";
		case STATE_GAME_MULTI:
			return "STATE_GAME_MULTI";
		case STATE_EXIT:
			return "STATE_EXIT";
	}
	return "UNKNOWN";
}

void _game_play(GAME_STATE *state)
{
	if (*state == STATE_GAME_SINGLE)
	{
		//game logic
	}
	else
	{
		log_output("game: Invalid gameplay state: %s\s", game_state_to_string(*state));
	}
}