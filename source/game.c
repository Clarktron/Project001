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

#define CORNER_LEFT (0x01)
#define CORNER_UP (0x02)
#define CORNER_RIGHT (0x04)
#define CORNER_DOWN (0x08)

#define TILE_WIDTH (40)
#define TILE_HEIGHT (20)
#define TILE_DEPTH (5)

#define SCREEN_SPEED_X (10)
#define SCREEN_SPEED_Y (5)

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
void _game_draw_tile(TILE tile, RENDER_S *render, int32_t x, int32_t y, uint64_t index);
void _game_scroll(GAME *game);

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
		input_register_mouse_button_event_cb(input, *_game_mouse_button_event_cb, NULL);
		input_register_mouse_motion_event_cb(input, *_game_mouse_motion_event_cb, game);
		*state = STATE_GAME_SINGLE_PLAY;
		log_output("game: State change (%s->%s)\n", game_state_to_string(STATE_GAME_SINGLE_INIT), game_state_to_string(*state));
	}
	else if (*state == STATE_GAME_MULTI_INIT)
	{
		input_register_mouse_button_event_cb(input, *_game_mouse_button_event_cb, NULL);
		input_register_mouse_motion_event_cb(input, *_game_mouse_motion_event_cb, game);
		*state = STATE_GAME_MULTI_PLAY;
		log_output("game: State change (%s->%s)\n", game_state_to_string(STATE_GAME_MULTI_INIT), game_state_to_string(*state));
	}
	else
	{
		log_output("game: Invalid gameplay state (%s)\n", game_state_to_string(*state));
	}

	memset(game, 0, sizeof(GAME));
	game->units = malloc(sizeof(UNIT) * NUM_UNITS);
	memset(game->units, 0, sizeof(UNIT) * NUM_UNITS);
	log_output("game: unit size: %i, list size: %i\n", sizeof(UNIT), sizeof(UNIT) * NUM_UNITS);
	_game_create_map_blank(game, 10, 10);
	game->x_offset = 0;
	game->y_offset = 0;
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
			map[index].base.corners = 0;
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
			int32_t x = i * (TILE_WIDTH / 2) + TILE_WIDTH / 2 + j * (TILE_WIDTH / 2);
			int32_t y = j * (TILE_HEIGHT / 2) - i * (TILE_HEIGHT / 2) + TILE_HEIGHT / 2 * game->map_width;
			_game_draw_tile(game->map[index], render, x - game->x_offset, y - game->y_offset, index);
		}
	}
}

void _game_draw_tile(TILE tile, RENDER_S *render, int32_t x, int32_t y, uint64_t index)
{
	int32_t x_left, x_up, x_right, x_down, y_left, y_up, y_right, y_down;

	uint8_t corners = tile.base.corners;

	x_left = x - TILE_WIDTH / 2;
	x_right = x + TILE_WIDTH / 2;
	x_up = x;
	x_down = x;

	if (corners & CORNER_LEFT)
	{
		y_left = y - TILE_DEPTH;
	}
	else
	{
		y_left = y;
	}

	if (corners & CORNER_RIGHT)
	{
		y_right = y - TILE_DEPTH;
	}
	else
	{
		y_right = y;
	}

	if (corners & CORNER_UP)
	{
		y_up = y - TILE_HEIGHT / 2 - TILE_DEPTH;
	}
	else
	{
		y_up = y - TILE_HEIGHT / 2;
	}

	if (corners & CORNER_DOWN)
	{
		y_down = y + TILE_HEIGHT / 2 - TILE_DEPTH;
	}
	else
	{
		y_down = y + TILE_HEIGHT / 2;
	}

	y_left -= tile.base.elevation * TILE_DEPTH;
	y_right -= tile.base.elevation * TILE_DEPTH;
	y_up -= tile.base.elevation * TILE_DEPTH;
	y_down -= tile.base.elevation * TILE_DEPTH;

	uint8_t r = 0, g = 0, b = 0;

	if (tile.type == TYPE_GRASS)
	{
		r = 0x2C;
		g = 0x78;
		b = 0x26;
	}
	else if (tile.type == TYPE_DIRT)
	{
		r = 0x8B;
		g = 0x76;
		b = 0x55;
	}

	render_line(render, x_left, y_left, x_up, y_up, r, g, b);
	render_line(render, x_up, y_up, x_right, y_right, r, g, b);
	render_line(render, x_right, y_right, x_down, y_down, r, g, b);
	render_line(render, x_down, y_down, x_left, y_left, r, g, b);
	char str[10];
	sprintf(str, "%llu", index);
	render_draw_text(render, x, y - tile.base.elevation * TILE_DEPTH, str, 1, r, g, b, 0xFF, ALIGN_CENTER, ALIGN_CENTER, QUALITY_BEST);
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