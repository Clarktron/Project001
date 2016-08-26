#ifndef GAME_H
#define GAME_H

#include "state.h"
#include "menu.h"
#include "render.h"
#include "unit.h"
#include "map.h"

#include <stdint.h>

typedef struct game GAME;

char *game_state_to_string(STATE state);
void game_loop(MENU_S *menu);

void game_unit_coords_to_drawing_coords(GAME *game, double unit_x, double unit_y, int32_t *screen_x, int32_t *screen_y);
void game_unit_coords_to_screen_coords(GAME *game, double unit_x, double unit_y, int32_t *screen_x, int32_t *screen_y);
void game_screen_coords_to_unit_coords(GAME *game, int32_t screen_x, int32_t screen_y, double *unit_x, double *unit_y);
void game_draw_unit_path(GAME *game, UNIT unit);
void game_draw_walls(GAME *game, uint8_t *walls, uint64_t wall_w, uint64_t wall_h);
void game_draw_nodes(GAME *game, void *mesh);

#endif