#ifndef GAME_H
#define GAME_H

#include <stdint.h>

typedef uint8_t GAME_STATE;
enum game_state {
	STATE_MENU_MAIN,
	STATE_MENU_SINGLE_OPTIONS,
	STATE_MENU_MULTI_OPTIONS,
	STATE_GAME_SINGLE,
	STATE_GAME_MULTI,
	STATE_EXIT
};

char *game_state_to_string(GAME_STATE state);
void game_loop();

#endif