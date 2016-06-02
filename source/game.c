#include "game.h"
#include "input.h"
#include "render.h"
#include "system.h"
#include "log.h"
#include "menu.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NUM_UNITS (100)

#define CORNER_FLAT (0x00)
#define CORNER_L (0x01)
#define CORNER_U (0x04)
#define CORNER_R (0x10)
#define CORNER_D (0x40)
#define CORNER_L2 (0x02)
#define CORNER_U2 (0x08)
#define CORNER_R2 (0x20)
#define CORNER_D2 (0x80)
#define CORNER_UR (CORNER_U | CORNER_R)
#define CORNER_LU (CORNER_L | CORNER_U)
#define CORNER_LD (CORNER_L | CORNER_D)
#define CORNER_RD (CORNER_R | CORNER_D)
#define CORNER_LR (CORNER_L | CORNER_R)
#define CORNER_UD (CORNER_U | CORNER_D)
#define CORNER_URD (CORNER_U | CORNER_R | CORNER_D)
#define CORNER_LRD (CORNER_L | CORNER_R | CORNER_D)
#define CORNER_LUD (CORNER_L | CORNER_U | CORNER_D)
#define CORNER_LUR (CORNER_L | CORNER_U | CORNER_R)
#define CORNER_UR2D (CORNER_U | CORNER_R2 | CORNER_D)
#define CORNER_LRD2 (CORNER_L | CORNER_R | CORNER_D2)
#define CORNER_L2UD (CORNER_L2 | CORNER_U | CORNER_D)
#define CORNER_LU2R (CORNER_L | CORNER_U2 | CORNER_R)

#define SCREEN_SPEED_X (10)
#define SCREEN_SPEED_Y (5)

#define MAP_FILE_VERSION (0)
#define MAP_NAME_LEN (260)

typedef enum unit_type
{
	TYPE_TANK,
	TYPE_GUNNER
} UNIT_TYPE;

typedef struct unit_base
{
	UNIT_TYPE type;
	double x, y;
	double speed;
	double accel;
	double size;
	uint64_t health;
	uint64_t attack;
	uint64_t defense;
} UNIT_BASE;

typedef struct unit_tank
{
	UNIT_TYPE type;
	double x, y;
	double speed;
	double accel;
	double size;
	uint64_t health;
	uint64_t attack;
	uint64_t defense;
	uint64_t bullet_speed;
	uint64_t accuracy;
} UNIT_TANK;

typedef struct unit_gunner
{
	UNIT_TYPE type;
	double x, y;
	double speed;
	double accel;
	double size;
	uint64_t health;
	uint64_t attack;
	uint64_t defense;
	uint64_t bullet_speed;
	uint64_t accuracy;
} UNIT_GUNNER;

typedef union unit
{
	UNIT_TYPE type;
	UNIT_BASE base;
	UNIT_TANK tank;
	UNIT_GUNNER gunner;
} UNIT;

typedef enum tile_type
{
	TYPE_GRASS,
	TYPE_DIRT,
	TYPE_WATER
} TILE_TYPE;

typedef struct tile_base
{
	TILE_TYPE type;
	uint32_t elevation;
	uint8_t corners; // 0000 | Left Up Right Down
} TILE_BASE;

typedef union tile
{
	TILE_TYPE type;
	TILE_BASE base;
} TILE;

typedef struct game
{
	int32_t mouse_x;
	int32_t mouse_y;
	int32_t x_offset;
	int32_t y_offset;
	UNIT *units;
	uint64_t num_units;
	TILE *map;
	uint64_t map_width;
	uint64_t map_height;
} GAME;

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
uint8_t _game_load_map(GAME *game, uint32_t number);

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
}

void _game_mouse_motion_event_cb(SDL_MouseMotionEvent ev, void *ptr)
{
	GAME *game = (GAME *)ptr;
	game->mouse_x = ev.x;
	game->mouse_y = ev.y;
}

void _game_init(GAME *game, RENDER_S *render, STATE *state, INPUT_S *input)
{
	if (*state == STATE_GAME_SINGLE_INIT)
	{
		input_register_mouse_button_event_cb(input, *_game_mouse_button_event_cb, game);
		input_register_mouse_motion_event_cb(input, *_game_mouse_motion_event_cb, game);
		*state = STATE_GAME_SINGLE_PLAY;
		log_output("game: State change (%s->%s)\n", game_state_to_string(STATE_GAME_SINGLE_INIT), game_state_to_string(*state));
	}
	else if (*state == STATE_GAME_MULTI_INIT)
	{
		input_register_mouse_button_event_cb(input, *_game_mouse_button_event_cb, game);
		input_register_mouse_motion_event_cb(input, *_game_mouse_motion_event_cb, game);
		*state = STATE_GAME_MULTI_PLAY;
		log_output("game: State change (%s->%s)\n", game_state_to_string(STATE_GAME_MULTI_INIT), game_state_to_string(*state));
	}
	else
	{
		log_output("game: Invalid gameplay state (%s)\n", game_state_to_string(*state));
		return;
	}

	memset(game, 0, sizeof(GAME));
	//game->units = malloc(sizeof(UNIT) * NUM_UNITS);
	//memset(game->units, 0, sizeof(UNIT) * NUM_UNITS);
	//log_output("game: unit size: %i, list size: %i\n", sizeof(UNIT), sizeof(UNIT) * NUM_UNITS);
	//_game_create_map_blank(game, 10, 10);
	if (_game_load_map(game, 1) != 0)
	{
		log_output("game: Failed to load map\n");
		*state = STATE_MENU_MAIN;
		return;
	}
}

void _game_play(GAME *game, RENDER_S *render, STATE *state)
{
	_game_scroll(game);
	_game_draw_map(game, render);
}

void _game_post(GAME *game, RENDER_S *render, STATE *state, INPUT_S *input)
{
	if (*state == STATE_GAME_SINGLE_POST)
	{
		input_unregister_mouse_button_event_cb(input);
		input_unregister_mouse_motion_event_cb(input);
		*state = STATE_MENU_SINGLE_OPTIONS;
		log_output("game: State change (%s->%s)\n", game_state_to_string(STATE_GAME_SINGLE_POST), game_state_to_string(*state));
	}
	else if (*state == STATE_GAME_MULTI_POST)
	{
		input_unregister_mouse_button_event_cb(input);
		input_unregister_mouse_motion_event_cb(input);
		*state = STATE_MENU_MULTI_OPTIONS;
		log_output("game: State change (%s->%s)\n", game_state_to_string(STATE_GAME_MULTI_POST), game_state_to_string(*state));
	}
	else
	{
		log_output("game: Invalid gameplay state (%s)\n", game_state_to_string(*state));
		return;
	}

	free(game->units);
	free(game->map);
}

void _game_create_map_blank(GAME *game, uint64_t width, uint64_t height)
{
	uint64_t i, j;
	TILE *map;
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
			map[index].base.elevation = 2;
			map[index].base.corners = CORNER_FLAT;
		}
	}
}

void _game_draw_map(GAME *game, RENDER_S *render)
{
	uint64_t i, j, k, l;

	uint64_t max;
	if (game->map_height > game->map_width)
	{
		max = game->map_width;
	}
	else
	{
		max = game->map_height;
	}
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
			int32_t y = (int32_t)(j * (TILE_HEIGHT / 2) - i * (TILE_HEIGHT / 2) + TILE_HEIGHT / 2 * game->map_width);
			_game_draw_tile(game->map[index], render, x - game->x_offset, y - game->y_offset, game->map[index].base.elevation);
		}
	}
}

uint8_t _game_corners_to_index(uint8_t corners)
{
	switch (corners)
	{
		case 0:
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
		case CORNER_LRD:
			return 9;
		case CORNER_LUD:
			return 10;
		case CORNER_LUR:
			return 11;
		case CORNER_URD:
			return 12;
		case CORNER_UR2D:
			return 13;
		case CORNER_LU2R:
			return 14;
		case CORNER_L2UD:
			return 15;
		case CORNER_LRD2:
			return 16;
		default:
			return (uint8_t)(-1);
	}
	return 0;
}

void _game_draw_tile(TILE tile, RENDER_S *render, int32_t x, int32_t y, uint64_t index)
{
	uint32_t i;

	// Draw the dirt foundation, bottom up
	for (i = 0; i < tile.base.elevation; i++)
	{
		render_draw_slope(render, 17, x, y - i * TILE_DEPTH);
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
	if (game->mouse_x < 0)
	{
		game->x_offset -= SCREEN_SPEED_X;
	}
	if (game->mouse_x >= SCREEN_WIDTH)
	{
		game->x_offset += SCREEN_SPEED_X;
	}
	if (game->mouse_y < 0)
	{
		game->y_offset -= SCREEN_SPEED_Y;
	}
	if (game->mouse_y >= SCREEN_HEIGHT)
	{
		game->y_offset += SCREEN_SPEED_Y;
	}

	// need to clamp the window to the border of the map
}

uint8_t _game_load_map(GAME *game, uint32_t number)
{
	char str[MAP_NAME_LEN];
	sprintf_s(str, sizeof(str), "resources\\%04lu.pmap", number);
	FILE *file;
	uint32_t version;
	uint64_t width = 0, height = 0;
	uint32_t i, j;

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