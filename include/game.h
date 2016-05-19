#ifndef GAME_H
#define GAME_H

#include "state.h"
#include "menu.h"
#include "render.h"

#include <stdint.h>

char *game_state_to_string(STATE state);
void game_loop(RENDER_S *render, MENU_S *menu);

#endif