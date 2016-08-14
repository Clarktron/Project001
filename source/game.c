#include "game.h"
#include "input.h"
#include "render.h"
#include "system.h"
#include "log.h"
#include "menu.h"
#include "unit.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NUM_UNITS (100)

#define SCREEN_SPEED_X (10)
#define SCREEN_SPEED_Y (5)

#define MAP_FILE_VERSION (0)
#define MAP_NAME_LEN (260)

void _game_mouse_button_event_cb(SDL_MouseButtonEvent ev, void *ptr);
void _game_mouse_motion_event_cb(SDL_MouseMotionEvent ev, void *ptr);
void _game_init(GAME *game, RENDER_S *render, STATE *state, INPUT_S *input);
void _game_play(GAME *game, RENDER_S *render, STATE *state);
void _game_post(GAME *game, RENDER_S *render, STATE *state, INPUT_S *input);
void _game_create_map_blank(GAME *game, uint64_t width, uint64_t height);
void _game_draw_map(GAME *game, RENDER_S *render);
uint8_t _game_corners_to_index(uint8_t corners);
void _game_draw_tile(TILE tile, RENDER_S *render, int32_t x, int32_t y, uint64_t index);
void _game_scroll(GAME *game);
void _game_draw_mouse(GAME *game, RENDER_S *render);
void _game_update_mouse(GAME *game);
uint8_t _game_load_map(GAME *game, uint32_t number);
void _game_unit_coords_to_screen_coords(GAME *game, double unit_x, double unit_y, int32_t *screen_x, int32_t *screen_y);
void _game_unit_base_coords_to_screen_coords(GAME *game, double unit_x, double unit_y, int32_t *screen_x, int32_t *screen_y);
void _game_screen_coords_to_unit_coords(GAME *game, int32_t screen_x, int32_t screen_y, double *unit_x, double *unit_y);
double _game_get_unit_z(double x, double y, uint8_t corners);
void _game_create_unit(GAME *game, UNIT unit);
void _game_create_unit_gunner(GAME *game, double x, double y);

void game_loop(RENDER_S *render, MENU_S *menu)
{
	STATE state = STATE_MENU_MAIN;
	GAME game;

	INPUT_S *input;

	input = input_setup();

	while (input_poll(input) == 0)
	{
		render_begin_frame(render);
		switch (state)
		{
			case STATE_MENU_MAIN:
			case STATE_MENU_SINGLE_OPTIONS:
			case STATE_MENU_MULTI_OPTIONS:
				menu_display(menu, render, input, &state);
				break;
			case STATE_GAME_SINGLE_INIT:
			case STATE_GAME_MULTI_INIT:
				_game_init(&game, render, &state, input);
				break;
			case STATE_GAME_SINGLE_PLAY:
			case STATE_GAME_MULTI_PLAY:
				_game_play(&game, render, &state);
				break;
			case STATE_GAME_SINGLE_POST:
			case STATE_GAME_MULTI_POST:
				_game_post(&game, render, &state, input);
				break;
			case STATE_EXIT:
				input_teardown(input);
				return;
		}
		render_end_frame(render);
	}

	input_teardown(input);
}

char *game_state_to_string(STATE state)
{
	switch (state)
	{
		case STATE_MENU_MAIN:
			return "STATE_MENU_MAIN";
		case STATE_MENU_SINGLE_OPTIONS:
			return "STATE_MENU_SINGLE_OPTIONS";
		case STATE_MENU_MULTI_OPTIONS:
			return "STATE_MENU_MULTI_OPTIONS";
		case STATE_GAME_SINGLE_INIT:
			return "STATE_GAME_SINGLE_INIT";
		case STATE_GAME_SINGLE_PLAY:
			return "STATE_GAME_SINGLE_PLAY";
		case STATE_GAME_SINGLE_POST:
			return "STATE_GAME_SINGLE_POST";
		case STATE_GAME_MULTI_INIT:
			return "STATE_GAME_MULTI_INIT";
		case STATE_GAME_MULTI_PLAY:
			return "STATE_GAME_MULTI_PLAY";
		case STATE_GAME_MULTI_POST:
			return "STATE_GAME_MULTI_POST";
		case STATE_EXIT:
			return "STATE_EXIT";
	}
	return "UNKNOWN";
}

void _game_mouse_button_event_cb(SDL_MouseButtonEvent ev, void *ptr)
{
	GAME *game = (GAME *)ptr;
	if (ev.button == SDL_BUTTON_LEFT)
	{
		if (ev.state == SDL_PRESSED)
		{
			game->mouse.left = MOUSE_DOWN;
			game->mouse.left_x = game->mouse.cur_x;
			game->mouse.left_y = game->mouse.cur_y;
		}
		else if (ev.state == SDL_RELEASED)
		{
			game->mouse.left = MOUSE_UP;
			UNIT_LIST *unit_list = game->unit_list;
			while (unit_list != NULL)
			{
				int32_t x, y;
				_game_unit_coords_to_screen_coords(game, unit_list->unit.base.x, unit_list->unit.base.y, &x, &y);
				if (((x >= game->mouse.left_x && x <= game->mouse.cur_x) || (x >= game->mouse.cur_x && x <= game->mouse.left_x)) && ((y >= game->mouse.left_y && y <= game->mouse.cur_y) || (y >= game->mouse.cur_y && y <= game->mouse.left_y)))
				{
					unit_list->unit.base.selected = 1;
				}
				else
				{
					unit_list->unit.base.selected = 0;
				}
				unit_list = unit_list->next;
			}
		}
	}
	else if (ev.button == SDL_BUTTON_RIGHT)
	{
		if (ev.state == SDL_PRESSED)
		{
			game->mouse.right = MOUSE_DOWN;
			game->mouse.right_x = game->mouse.cur_x;
			game->mouse.right_y = game->mouse.cur_y;
		}
		else if (ev.state == SDL_RELEASED)
		{
			game->mouse.right = MOUSE_UP;
			UNIT_LIST *unit_list = game->unit_list;
			while (unit_list != NULL)
			{
				if (unit_list->unit.base.selected == 1)
				{
					unit_pathfind(unit_list->unit, game->mouse.cur_x, game->mouse.cur_y);
				}
				unit_list = unit_list->next;
			}
		}
	}
}

void _game_mouse_motion_event_cb(SDL_MouseMotionEvent ev, void *ptr)
{
	GAME *game = (GAME *)ptr;
	game->mouse.cur_x = ev.x / SCALE_X;
	game->mouse.cur_y = ev.y / SCALE_Y;
}

void _game_keboard_event_cb(SDL_KeyboardEvent ev, void *ptr)
{
	GAME *game = (GAME *)ptr;

	if (ev.keysym.sym == SDLK_ESCAPE && ev.state == SDL_PRESSED)
	{
		game->quit = 1;
	}
}

void _game_init(GAME *game, RENDER_S *render, STATE *state, INPUT_S *input)
{
	uint32_t i, j;

	if (*state == STATE_GAME_SINGLE_INIT)
	{
		input_register_keyboard_event_cb(input, *_game_keboard_event_cb, game);
		input_register_mouse_button_event_cb(input, *_game_mouse_button_event_cb, game);
		input_register_mouse_motion_event_cb(input, *_game_mouse_motion_event_cb, game);
		*state = STATE_GAME_SINGLE_PLAY;
		log_output("game: State change (%s->%s)\n", game_state_to_string(STATE_GAME_SINGLE_INIT), game_state_to_string(*state));
	}
	else
	{
		log_output("game: Invalid gameplay state (%s)\n", game_state_to_string(*state));
		return;
	}

	memset(game, 0, sizeof(GAME));


	//* random map generation
	game->map_height = 10;
	game->map_width = 10;

	game->map = malloc(sizeof(TILE) * game->map_width * game->map_height);
	memset(game->map, 0, sizeof(TILE) * game->map_width * game->map_height);
	for (j = 0; j < game->map_height; ++j)
	{
		for (i = 0; i < game->map_width; ++i)
		{
			uint64_t index = i + j * game->map_width;
			game->map[index].type = 0;
			game->map[index].base.elevation = system_rand() % 3;
			game->map[index].base.corners = 0;
			game->map[index].base.num_units = 0;
			if (system_rand() % 2 == 0)
			{
				game->map[index].base.corners |= (system_rand() % 2 == 0) ? (system_rand() % 2 == 0) ? CORNER_L : CORNER_R : (system_rand() % 2 == 0) ? CORNER_U : CORNER_D;
			}
			if (system_rand() % 2 == 0)
			{
				game->map[index].base.corners |= (system_rand() % 2 == 0) ? (system_rand() % 2 == 0) ? CORNER_L : CORNER_R : (system_rand() % 2 == 0) ? CORNER_U : CORNER_D;
			}
		}
	}
	//*/

	/* load map from file
	if (_game_load_map(game, 1) != 0)
	{
		log_output("game: Failed to load map\n");
		*state = STATE_MENU_MAIN;
		return;
	}
	//*/

	// create blank map
	//_game_create_map_blank(game, 10, 10);
	
	game->x_offset = (int32_t)((game->map_width * (TILE_WIDTH / 2) + game->map_height * (TILE_WIDTH / 2)) / 2 - (SCREEN_WIDTH / 2));
	game->y_offset = (int32_t)(-1 * (SCREEN_HEIGHT / 2) + (game->map_height * (TILE_HEIGHT / 2) - game->map_width * (TILE_HEIGHT / 2)) / 2);
	game->mouse.left = MOUSE_UP;
	game->mouse.prev_left = MOUSE_UP;
	game->mouse.left = MOUSE_UP;
	game->mouse.prev_right = MOUSE_UP;
	SDL_GetMouseState(&(game->mouse.cur_x), &(game->mouse.cur_y));
	game->mouse.cur_x /= SCALE_X;
	game->mouse.cur_y /= SCALE_Y;

	/*/
	UNIT unit;
	unit.base = unit_create_base(0.0, 0.0, 1, 0, 0.1, 5, 100, 100, 1, 1);
	unit.gunner = unit_create_gunner(unit.base, 1, 100);
	int32_t x, y;
	x = (int32_t)unit.base.x;
	y = (int32_t)unit.base.y;
	unit_insert(&(game->unit_list), &(game->unit_list_end), unit);
	game->map[x + y * game->map_width].base.num_units++;
	//*/

	//*
	double scale_x = 10;
	double scale_y = 10;
	for (j = 0; j < game->map_height * scale_y; ++j)
	{
		for (i = 0; i < game->map_width * scale_x; ++i)
		{
			_game_create_unit_gunner(game, i / scale_x + (1 / scale_x) / 2.0, j / scale_y + (1 / scale_y) / 2.0);
		}
	}
	//*/
}

void _game_play(GAME *game, RENDER_S *render, STATE *state)
{
	_game_update_mouse(game);
	_game_scroll(game);

	unit_msort_unit_list(&(game->unit_list), &(game->unit_list_end));

	_game_draw_map(game, render);
	_game_draw_mouse(game, render);

	if (game->quit)
	{
		game->quit = 0;
		*state = STATE_GAME_SINGLE_POST;
		log_output("game: State change (%s->%s)\n", game_state_to_string(STATE_GAME_SINGLE_PLAY), game_state_to_string(*state));
	}
}

void _game_post(GAME *game, RENDER_S *render, STATE *state, INPUT_S *input)
{
	if (*state == STATE_GAME_SINGLE_POST)
	{
		input_unregister_keyboard_event_cb(input);
		input_unregister_mouse_button_event_cb(input);
		input_unregister_mouse_motion_event_cb(input);
		*state = STATE_MENU_SINGLE_OPTIONS;
		log_output("game: State change (%s->%s)\n", game_state_to_string(STATE_GAME_SINGLE_POST), game_state_to_string(*state));
	}
	else
	{
		log_output("game: Invalid gameplay state (%s)\n", game_state_to_string(*state));
		return;
	}

	UNIT_LIST *unit_list = game->unit_list;

	while (unit_list != NULL)
	{
		UNIT_LIST *next = unit_list->next;

		free(unit_list);

		unit_list = next;
	}

	free(game->map);
}

void _game_create_map_blank(GAME *game, uint64_t width, uint64_t height)
{
	uint64_t i, j;
	TILE *map;
	if (width == 0 || height == 0)
	{
		return;
	}
	if ((game->map = malloc(sizeof(TILE) * width * height)) == NULL)
	{
		log_output("game: Insufficient memory\n");
		return;
	}
	game->map_width = width;
	game->map_height = height;
	map = game->map;
	memset(map, 0, sizeof(TILE) * width * height);
	for (j = 0; j < height; ++j)
	{
		for (i = 0; i < width; ++i)
		{
			uint64_t index = i + j * width;
			map[index].type = TYPE_GRASS;
			map[index].base.elevation = 3;
			map[index].base.corners = CORNER_FLAT;
		}
	}
}

void _game_draw_map(GAME *game, RENDER_S *render)
{
	uint64_t i, j, k, l, m;

	uint64_t max;
	if (game->map_height == 0 || game->map_width == 0)
	{
		return;
	}
	if (game->map_height > game->map_width)
	{
		max = game->map_width;
	}
	else
	{
		max = game->map_height;
	}
	uint32_t count = 0;
	UNIT_LIST *top = game->unit_list;
	for (l = 0; l < game->map_height + game->map_width - 1; ++l)
	{
		for (k = 0; k < max && k < l + 1 && k < game->map_height + game->map_width - l - 1; k++)
		{
			if (l < game->map_width)
			{
				i = game->map_width - 1 - l + k;
				j = k;
			}
			else
			{
				i = k;
				j = l - game->map_width + k + 1;
			}
			uint64_t index = i + j * game->map_width;
			int32_t x = (int32_t)(i * (TILE_WIDTH / 2) + TILE_WIDTH / 2 + j * (TILE_WIDTH / 2));
			int32_t y = (int32_t)(j * (TILE_HEIGHT / 2) - i * (TILE_HEIGHT / 2));
			_game_draw_tile(game->map[index], render, x - game->x_offset, y - game->y_offset, game->map[index].base.elevation);
			for (m = 0; m < game->map[index].base.num_units; ++m)
			{
				if (top == NULL)
				{
					break;
				}
				double x = top->unit.base.x;
				double y = top->unit.base.y;

				int32_t x_coord, y_coord;
				_game_unit_coords_to_screen_coords(game, x, y, &x_coord, &y_coord);

				render_draw_unit(render, 0, x_coord, y_coord);
				if (top->unit.base.selected)
				{
					render_draw_unit(render, 1, x_coord, y_coord);
				}
				/*
				char str[10];
				sprintf(str, "%lu", count++);
				render_draw_text(render, x_coord, y_coord, str, 1, 0x56, 0xB8, 0xFF, 0xFF, ALIGN_CENTER, ALIGN_TOP, QUALITY_BEST);
				//*/
				top = top->next;
			}
		}
	}
}

uint8_t _game_corners_to_index(uint8_t corners)
{
	switch (corners)
	{
		case CORNER_FLAT:
			return 0;
		case CORNER_L:
			return 1;
		case CORNER_R:
			return 2;
		case CORNER_U:
			return 3;
		case CORNER_D:
			return 4;
		case CORNER_LU:
			return 5;
		case CORNER_UR:
			return 6;
		case CORNER_RD:
			return 7;
		case CORNER_LD:
			return 8;
		case CORNER_LR:
			return 9;
		case CORNER_UD:
			return 10;
		case CORNER_LRD:
			return 11;
		case CORNER_LUD:
			return 12;
		case CORNER_LUR:
			return 13;
		case CORNER_URD:
			return 14;
		case CORNER_UR2D:
			return 15;
		case CORNER_LU2R:
			return 16;
		case CORNER_L2UD:
			return 17;
		case CORNER_LRD2:
			return 18;
		case CORNER_BASE:
			return 19;
		default:
			return 0;
	}
	return 0;
}

void _game_draw_tile(TILE tile, RENDER_S *render, int32_t x, int32_t y, uint64_t index)
{
	uint32_t i;

	// Draw the dirt foundation, bottom up
	for (i = 0; i < tile.base.elevation; i++)
	{
		render_draw_slope(render, _game_corners_to_index(CORNER_BASE), x, y - i * TILE_DEPTH);
	}
	// ---

	// Draw the tile surface
	render_draw_slope(render, _game_corners_to_index(tile.base.corners), x, y - tile.base.elevation * TILE_DEPTH);
	// ---

	// Draw the tile number (elevation, draw index, etc)
	/*
	char str[10];
	sprintf(str, "%llu", index);
	render_draw_text(render, x, y - tile.base.elevation * TILE_DEPTH, str, 1, 0x56, 0xB8, 0xFF, 0xFF, ALIGN_CENTER, ALIGN_CENTER, QUALITY_BEST);
	//*/
	// ---
}

void _game_scroll(GAME *game)
{
	if (game->mouse.left == MOUSE_UP)
	{
		if (game->mouse.cur_x == 0)
		{
			game->x_offset -= SCREEN_SPEED_X;
		}
		if (game->mouse.cur_x == SCREEN_WIDTH - 1)
		{
			game->x_offset += SCREEN_SPEED_X;
		}
		if (game->mouse.cur_y == 0)
		{
			game->y_offset -= SCREEN_SPEED_Y;
		}
		if (game->mouse.cur_y == SCREEN_HEIGHT - 1)
		{
			game->y_offset += SCREEN_SPEED_Y;
		}
		// need to clamp the window to the border of the map
		
		double x, y;
		uint8_t clamp = 0;

		_game_screen_coords_to_unit_coords(game, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, &x, &y);

		double orig_x = x;
		double orig_y = y;

		if (x < 0.0)
		{
			clamp = 1;
			x = 0.0;
		}
		if (x > game->map_width)
		{
			clamp = 1;
			x = (double)game->map_width;
		}
		if (y < 0.0)
		{
			clamp = 1;
			y = 0.0;
		}
		if (y > game->map_height)
		{
			clamp = 1;
			y = (double)game->map_height;
		}

		if (clamp)
		{
			int32_t screen_x, screen_y;
			int32_t orig_screen_x, orig_screen_y;

			_game_unit_base_coords_to_screen_coords(game, x, y, &screen_x, &screen_y);
			_game_unit_base_coords_to_screen_coords(game, orig_x, orig_y, &orig_screen_x, &orig_screen_y);

			int32_t diff_x = screen_x - orig_screen_x;
			int32_t diff_y = screen_y - orig_screen_y;

			game->x_offset += diff_x;
			game->y_offset += diff_y;
		}
	}
}

void _game_draw_mouse(GAME *game, RENDER_S *render)
{
	if (game->mouse.left == MOUSE_DOWN)
	{
		render_rectangle(render, game->mouse.left_x, game->mouse.left_y, game->mouse.cur_x - game->mouse.left_x + 1, game->mouse.cur_y - game->mouse.left_y + 1, 0x00, 0xFF, 0x00, 0xFF, OUTLINE);
	}
}

void _game_update_mouse(GAME *game)
{
	if (game->mouse.cur_x < 0)
	{
		game->mouse.cur_x = 0;
	}
	else if (game->mouse.cur_x >= SCREEN_WIDTH)
	{
		game->mouse.cur_x = SCREEN_WIDTH - 1;
	}

	if (game->mouse.cur_y < 0)
	{
		game->mouse.cur_y = 0;
	}
	else if (game->mouse.cur_y >= SCREEN_HEIGHT)
	{
		game->mouse.cur_y = SCREEN_HEIGHT - 1;
	}

	if (game->mouse.left == MOUSE_DOWN && game->mouse.prev_left == MOUSE_UP)
	{
		game->mouse.prev_left = game->mouse.left;
	}
	else if (game->mouse.left == MOUSE_UP && game->mouse.prev_left == MOUSE_DOWN)
	{
		game->mouse.prev_left = game->mouse.left;
	}

	if (game->mouse.right == MOUSE_DOWN && game->mouse.prev_right == MOUSE_UP)
	{
		game->mouse.prev_right = game->mouse.right;
	}
	else if (game->mouse.right == MOUSE_UP && game->mouse.prev_right == MOUSE_DOWN)
	{
		game->mouse.prev_right = game->mouse.right;
	}
}

uint8_t _game_load_map(GAME *game, uint32_t number)
{
	char str[MAP_NAME_LEN];
	FILE *file;
	uint32_t version;
	uint64_t width = 0, height = 0;
	uint32_t i, j;

	sprintf_s(str, sizeof(str), "resources\\%04lu.pmap", number);

	file = fopen(str, "r");
	if (file == NULL)
	{
		log_output("game: Could not open file %s, likely does not exist\n", str);
		return 1;
	}

	if (fscanf(file, "%lu", &version) < 1)
	{
		log_output("game: Could not read verion number. feof: %i ferror: %i\n", feof(file), ferror(file));
		return 1;
	}
	if (version == 0)
	{
		if (fscanf(file, "%llu %llu", &width, &height) < 2)
		{
			log_output("game: Could not read width & height from file. feof: %i ferror: %i\n", feof(file), ferror(file));
			return 1;
		}
		game->map = malloc(sizeof(TILE) * width * height);
		memset(game->map, 0, sizeof(TILE) * width * height);
		game->map_width = width;
		game->map_height = height;
		for (j = 0; j < height; ++j)
		{
			for (i = 0; i < width; ++i)
			{
				if (fscanf(file, "%lu", &(game->map[i + j * width].base.elevation)) < 1)
				{
					log_output("game: Could not read data from file. feof: %i ferror: %i\n", feof(file), ferror(file));
					return 1;
				}
			}
		}
	}
	else if (version == 1)
	{
		if (fscanf(file, "%llu %llu", &width, &height) < 2)
		{
			log_output("game: Could not read width & height from file. feof: %i ferror: %i\n", feof(file), ferror(file));
			return 1;
		}
		if ((game->map = malloc(sizeof(TILE) * width * height)) == NULL)
		{
			log_output("game: Insufficient memory\n");
			fclose(file);
			return 1;
		}

		memset(game->map, 0, sizeof(TILE) * width * height);
		game->map_width = width;
		game->map_height = height;
		for (j = 0; j < height; ++j)
		{
			for (i = 0; i < width; ++i)
			{
				if (fscanf(file, "%lu%lu%hhx,", &(game->map[i + j * width].base.type), &(game->map[i + j * width].base.elevation), &(game->map[i + j * width].base.corners)) < 3)
				{
					log_output("game: Could not read data from file. feof: %i ferror: %i\n", feof(file), ferror(file));
					return 1;
				}
			}
		}
	}

	fclose(file);
	return 0;
}

void _game_unit_coords_to_screen_coords(GAME *game, double unit_x, double unit_y, int32_t *screen_x, int32_t *screen_y)
{
	int32_t elevation = 0;
	if (screen_x == NULL || screen_y == NULL)
	{
		log_output("game: Parameter was NULL: screen_x = %i, screen_y = %i\n", screen_x, screen_y);
		return;
	}
	if (((int32_t)unit_x >= 0 && (int32_t)unit_x < game->map_width) && ((int32_t)unit_y >= 0 && (int32_t)unit_y < game->map_height))
	{
		elevation = game->map[(int32_t)unit_x + (int32_t)unit_y * game->map_width].base.elevation;
	}
	*screen_x = (int32_t)round(unit_x * (TILE_WIDTH / 2) + unit_y * (TILE_WIDTH / 2)) - game->x_offset;
	*screen_y = (int32_t)round(unit_y * (TILE_HEIGHT / 2) - unit_x * (TILE_HEIGHT / 2)) - game->y_offset - elevation * TILE_DEPTH - UNIT_HEIGHT / 2 - (int32_t)round(_game_get_unit_z(unit_x, unit_y, game->map[(int32_t)unit_x + (int32_t)unit_y * game->map_width].base.corners) * TILE_DEPTH);
}

void _game_unit_base_coords_to_screen_coords(GAME *game, double unit_x, double unit_y, int32_t *screen_x, int32_t *screen_y)
{
	int32_t elevation = 0;
	if (screen_x == NULL || screen_y == NULL)
	{
		log_output("game: Parameter was NULL: screen_x = %i, screen_y = %i\n", screen_x, screen_y);
		return;
	}
	*screen_x = (int32_t)round(unit_x * (TILE_WIDTH / 2) + unit_y * (TILE_WIDTH / 2)) - game->x_offset;
	*screen_y = (int32_t)round(unit_y * (TILE_HEIGHT / 2) - unit_x * (TILE_HEIGHT / 2)) - game->y_offset;
}

void _game_screen_coords_to_unit_coords(GAME *game, int32_t screen_x, int32_t screen_y, double *unit_x, double *unit_y)
{
	if (unit_x == NULL || unit_y == NULL)
	{
		log_output("game: Parameter was NULL: unit_x = %i, unit_y = %i\n", unit_x, unit_y);
		return;
	}
	double x_orig = screen_x + game->x_offset;
	double y_orig = screen_y + game->y_offset;

	*unit_x = x_orig / TILE_WIDTH - y_orig / TILE_HEIGHT;
	*unit_y = y_orig / TILE_HEIGHT + x_orig / TILE_WIDTH;
}

double _game_get_unit_z(double x, double y, uint8_t corners)
{
	x = fmod(x, 1);
	y = fmod(y, 1);
	switch (corners)
	{
	case CORNER_FLAT:
		return (0.0);
	case CORNER_L:
		if ((-x + 1.0) + (-y + 1.0) > 1.0)
		{
			return ((-x + 1.0) + (-y + 1.0) - 1.0);
		}
		return (0.0);
	case CORNER_R:
		if (x + y > 1.0)
		{
			return (x + y - 1.0);
		}
		return (0.0);
	case CORNER_U:
		if (x + (-y + 1.0) > 1.0)
		{
			return (x + (-y + 1.0) - 1.0);
		}
		return (0.0);
	case CORNER_D:
		if ((-x + 1.0) + y > 1.0)
		{
			return ((-x + 1.0) + y - 1.0);
		}
		return (0.0);
	case CORNER_LU:
		return (-y + 1.0);
	case CORNER_UR:
		return (x);
	case CORNER_RD:
		return (y);
	case CORNER_LD:
		return (-x + 1.0);
	case CORNER_LR:
		if ((-x + 1.0) + (-y + 1.0) > 1.0)
		{
			return ((-x + 1.0) + (-y + 1.0) - 1.0);
		}
		return (x + y - 1.0);
	case CORNER_UD:
		if (x + (-y + 1.0) > 1.0)
		{
			return (x + (-y + 1.0) - 1.0);
		}
		return ((-x + 1.0) + y - 1.0);
	case CORNER_LRD:
		if (x + (-y + 1.0) > 1.0)
		{
			return (1.0);
		}
		return ((-x + 1.0) + y - 1.0);
	case CORNER_LUD:
		if (x + y > 1.0)
		{
			return (1.0);
		}
		return ((-x + 1.0) + (-y + 1.0));
	case CORNER_LUR:
		if ((-x + 1.0) + y > 1.0)
		{
			return (1.0);
		}
		return (x + (-y + 1.0) - 1.0);
	case CORNER_URD:
		if ((-x + 1.0) + (-y + 1.0) > 1.0)
		{
			return (1.0);
		}
		return (x + y - 1.0);
	case CORNER_UR2D:
		return (x + y);
	case CORNER_LU2R:
		return (x + (-y + 1.0));
	case CORNER_L2UD:
		return ((-x + 1.0) + (-y + 1.0));
	case CORNER_LRD2:
		return ((-x + 1.0) + y);
	case CORNER_BASE:
	default:
		return (0.0);
	}
	return (0.0);
}

void _game_create_unit(GAME *game, UNIT unit)
{
	int32_t x_grid, y_grid;
	x_grid = (int32_t)unit.base.x;
	y_grid = (int32_t)unit.base.y;
	if (game->map[x_grid + y_grid * game->map_width].base.num_units < 0xFFFFFFFFFFFFFFFF)
	{
		unit_insert(&(game->unit_list), &(game->unit_list_end), unit);
		game->map[x_grid + y_grid * game->map_width].base.num_units++;
	}
	else
	{
		// throw some sort of error
	}
}

void _game_create_unit_gunner(GAME *game, double x, double y)
{
	UNIT unit;

	// --- //
	unit.base = unit_create_base(x, y, 0.1, 0.0, 0.1, 5.0, 100, 100, 2, 2);
	unit.gunner = unit_create_gunner(unit.base, 10, 75);
	// --- //

	_game_create_unit(game, unit);
}