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
#define CORNER_BASE (CORNER_L | CORNER_U | CORNER_R | CORNER_D | CORNER_L2 | CORNER_U2 | CORNER_R2 | CORNER_D2)

#define SCREEN_SPEED_X (10)
#define SCREEN_SPEED_Y (5)

#define MAP_FILE_VERSION (0)
#define MAP_NAME_LEN (260)

// ----------------------------------------------------------------------------
// Unit info

typedef enum unit_type
{
	TYPE_TANK,
	TYPE_GUNNER
} UNIT_TYPE;

typedef struct unit_base
{
	UNIT_TYPE type;
	uint8_t selected;
	double x, y;
	double max_speed;
	double speed;
	double accel;
	double size;
	uint64_t max_health;
	uint64_t health;
	uint64_t attack;
	uint64_t defense;
} UNIT_BASE;

typedef struct unit_tank
{
	UNIT_TYPE type;
	uint8_t selected;
	double x, y;
	double max_speed;
	double speed;
	double accel;
	double size;
	uint64_t max_health;
	uint64_t health;
	uint64_t attack;
	uint64_t defense;
	uint64_t bullet_speed;
	uint64_t accuracy;
} UNIT_TANK;

typedef struct unit_gunner
{
	UNIT_TYPE type;
	uint8_t selected;
	double x, y;
	double max_speed;
	double speed;
	double accel;
	double size;
	uint64_t max_health;
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

// ----------------------------------------------------------------------------
// Tile info

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
	uint8_t corners; // 0000 | LURD
	uint8_t num_units;
} TILE_BASE;

typedef union tile
{
	TILE_TYPE type;
	TILE_BASE base;
} TILE;

// ----------------------------------------------------------------------------

typedef struct unit_list UNIT_LIST;

struct unit_list
{
	UNIT_LIST *next;
	UNIT_LIST *prev;
	UNIT unit;
};

typedef enum mouse_state
{
	MOUSE_DOWN,
	MOUSE_UP
} MOUSE_STATE;

typedef struct mouse
{
	MOUSE_STATE prev_state;
	MOUSE_STATE state;
	int32_t down_x;
	int32_t down_y;
	int32_t cur_x;
	int32_t cur_y;
} MOUSE;

typedef struct game
{
	MOUSE mouse;
	int32_t x_offset;
	int32_t y_offset;
	UNIT_LIST *unit_list;
	UNIT_LIST *unit_list_end;
	uint64_t num_units;
	TILE *map;
	uint64_t map_width;
	uint64_t map_height;
	uint8_t quit;
} GAME;

void _game_mouse_button_event_cb(SDL_MouseButtonEvent ev, void *ptr);
void _game_mouse_motion_event_cb(SDL_MouseMotionEvent ev, void *ptr);
void _game_init(GAME *game, RENDER_S *render, STATE *state, INPUT_S *input);
void _game_play(GAME *game, RENDER_S *render, STATE *state);
void _game_post(GAME *game, RENDER_S *render, STATE *state, INPUT_S *input);
void _game_create_map_blank(GAME *game, uint64_t width, uint64_t height);
void _game_draw_map(GAME *game, RENDER_S *render);
void _game_draw_units(GAME *game, RENDER_S *render);
void _game_unit_coords_to_screen_coords(GAME *game, double unit_x, double unit_y, int32_t *screen_x, int32_t *screen_y);
uint8_t _game_corners_to_index(uint8_t corners);
void _game_draw_tile(TILE tile, RENDER_S *render, int32_t x, int32_t y, uint64_t index);
void _game_scroll(GAME *game);
void _game_draw_mouse(GAME *game, RENDER_S *render);
void _game_update_mouse(GAME *game);
uint8_t _game_load_map(GAME *game, uint32_t number);
UNIT_BASE _game_create_unit_base(double x, double y, double max_speed, double speed, double accel, double size, uint64_t max_health, uint64_t health, uint64_t attack, uint64_t defense);
UNIT_GUNNER _game_create_unit_gunner(UNIT_BASE base, uint64_t bullet_speed, uint64_t accuracy);
void _game_insert_unit(GAME *game, UNIT unit);

void _game_msort_unit_list(GAME *game);
UNIT_LIST *_game_msort_unit_compare(UNIT_LIST *first, UNIT_LIST *second);
UNIT_LIST *_game_msort_unit_merge(UNIT_LIST *first, UNIT_LIST *second);

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
	if (ev.button == SDL_BUTTON_LEFT && ev.state == SDL_PRESSED)
	{
		game->mouse.state = MOUSE_DOWN;
		game->mouse.down_x = game->mouse.cur_x;
		game->mouse.down_y = game->mouse.cur_y;
	}
	else if (ev.button == SDL_BUTTON_LEFT && ev.state == SDL_RELEASED)
	{
		game->mouse.state = MOUSE_UP;
		UNIT_LIST *unit_list = game->unit_list;
		while (unit_list != NULL)
		{
			int32_t x, y;
			_game_unit_coords_to_screen_coords(game, unit_list->unit.base.x, unit_list->unit.base.y, &x, &y);
			if (((x >= game->mouse.down_x && x <= game->mouse.cur_x) || (x >= game->mouse.cur_x && x <= game->mouse.down_x)) && ((y >= game->mouse.down_y && y <= game->mouse.cur_y) || (y >= game->mouse.cur_y && y <= game->mouse.down_y)))
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

	game->x_offset = (int32_t)(game->map_width * (TILE_WIDTH / 2) - (SCREEN_WIDTH / 2));
	game->y_offset = -1 * (SCREEN_HEIGHT / 2);
	game->mouse.state = MOUSE_UP;
	game->mouse.prev_state = MOUSE_UP;
	SDL_GetMouseState(&(game->mouse.cur_x), &(game->mouse.cur_y));

	//*
	for (i = 0; i < 100; ++i)
	{
		UNIT unit;
		unit.base = _game_create_unit_base((system_rand() % (game->map_width * 10)) / 10.0, (system_rand() % (game->map_height * 10)) / 10.0, 1, 0, 0.1, 5, 100, 100, 1, 1);
		unit.gunner = _game_create_unit_gunner(unit.base, 1, 100);
		_game_insert_unit(game, unit);
	}
	//*/
}

void _game_play(GAME *game, RENDER_S *render, STATE *state)
{
	_game_update_mouse(game);
	_game_scroll(game);

	_game_msort_unit_list(game);

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
				sprintf(str, "%llu", count++);
				render_draw_text(render, x_coord, y_coord, str, 1, 0x56, 0xB8, 0xFF, 0xFF, ALIGN_CENTER, ALIGN_TOP, QUALITY_BEST);
				//*/
				top = top->next;
			}
		}
	}
}

void _game_draw_units(GAME *game, RENDER_S *render)
{
	UNIT_LIST *unit_list = game->unit_list;
	uint32_t index = 0;

	while (unit_list != NULL)
	{
		double x = unit_list->unit.base.x;
		double y = unit_list->unit.base.y;

		int32_t x_coord, y_coord;
		_game_unit_coords_to_screen_coords(game, x, y, &x_coord, &y_coord);

		render_draw_unit(render, 0, x_coord, y_coord);
		char str[10];
		sprintf(str, "%lu", index++);
		render_draw_text(render, x_coord, y_coord, str, 1, 0x56, 0xB8, 0xFF, 0xFF, ALIGN_CENTER, ALIGN_TOP, QUALITY_BEST);

		unit_list = unit_list->next;
	}
	
	unit_list = game->unit_list;

	while (unit_list != NULL)
	{
		double x = unit_list->unit.base.x;
		double y = unit_list->unit.base.y;

		int32_t x_coord, y_coord;
		_game_unit_coords_to_screen_coords(game, x, y, &x_coord, &y_coord);

		if (unit_list->unit.base.selected)
		{
			render_draw_unit(render, 1, x_coord, y_coord);
		}

		unit_list = unit_list->next;
	}
}

void _game_unit_coords_to_screen_coords(GAME *game, double unit_x, double unit_y, int32_t *screen_x, int32_t *screen_y)
{
	int32_t elevation = 0;
	if (((int32_t)unit_x >= 0 && (int32_t)unit_x < game->map_width) && ((int32_t)unit_y >= 0 && (int32_t)unit_y < game->map_height))
	{
		elevation = game->map[(int32_t)unit_x + (int32_t)unit_y * game->map_width].base.elevation;
	}
	*screen_x = (int32_t)round(unit_x * (TILE_WIDTH / 2) + unit_y * (TILE_WIDTH / 2)) - game->x_offset;
	*screen_y = (int32_t)round(unit_y * (TILE_HEIGHT / 2) - unit_x * (TILE_HEIGHT / 2)) - game->y_offset - elevation * TILE_DEPTH - UNIT_HEIGHT / 2;
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
	if (game->mouse.state == MOUSE_UP)
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
	}
}

void _game_draw_mouse(GAME *game, RENDER_S *render)
{
	if (game->mouse.state == MOUSE_DOWN)
	{
		render_rectangle(render, game->mouse.down_x, game->mouse.down_y, game->mouse.cur_x - game->mouse.down_x + 1, game->mouse.cur_y - game->mouse.down_y + 1, 0x00, 0xFF, 0x00, 0xFF, OUTLINE);
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

	if (game->mouse.state == MOUSE_DOWN && game->mouse.prev_state == MOUSE_UP)
	{
		game->mouse.prev_state = game->mouse.state;
	}
	else if (game->mouse.state == MOUSE_UP && game->mouse.prev_state == MOUSE_DOWN)
	{
		game->mouse.prev_state = game->mouse.state;
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

UNIT_BASE _game_create_unit_base(double x, double y, double max_speed, double speed, double accel, double size, uint64_t max_health, uint64_t health, uint64_t attack, uint64_t defense)
{
	UNIT_BASE new_unit;

	new_unit.selected = 0;
	new_unit.x = x;
	new_unit.y = y;
	new_unit.max_speed = max_speed;
	new_unit.speed = speed;
	new_unit.accel = accel;
	new_unit.size = size;
	new_unit.max_health = max_health;
	new_unit.health = health;
	new_unit.attack = attack;
	new_unit.defense = defense;

	return new_unit;
}

UNIT_GUNNER _game_create_unit_gunner(UNIT_BASE base, uint64_t bullet_speed, uint64_t accuracy)
{
	UNIT_GUNNER new_unit;

	new_unit.selected = base.selected;
	new_unit.x = base.x;
	new_unit.y = base.y;
	new_unit.max_speed = base.max_speed;
	new_unit.speed = base.speed;
	new_unit.accel = base.accel;
	new_unit.size = base.size;
	new_unit.max_health = base.max_health;
	new_unit.health = base.health;
	new_unit.attack = base.attack;
	new_unit.defense = base.defense;

	new_unit.type = TYPE_GUNNER;
	new_unit.bullet_speed = bullet_speed;
	new_unit.accuracy = accuracy;

	return new_unit;
}

void _game_insert_unit(GAME *game, UNIT new_unit)
{
	UNIT_LIST *unit_list = game->unit_list; //point to the head of the linked list

	if (unit_list == NULL)
	{
		game->unit_list = malloc(sizeof(UNIT_LIST));
		game->unit_list->next = NULL;
		game->unit_list->prev = NULL;
		game->unit_list->unit = new_unit;
		game->unit_list_end = game->unit_list;
	}
	else
	{
		if (game->unit_list_end != NULL)
		{
			unit_list = game->unit_list_end;
		}
		else
		{
			log_output("game: Failed to use cached unit list ending, parsing through instead");
			while (unit_list->next != NULL)
			{
				unit_list = unit_list->next;
			}
		}
		unit_list->next = malloc(sizeof(UNIT_LIST));
		unit_list->next->next = NULL;
		unit_list->next->prev = unit_list;
		unit_list->next->unit = new_unit;
		game->unit_list_end = unit_list->next;
	}

	int32_t x, y;
	x = (int32_t)new_unit.base.x;
	y = (int32_t)new_unit.base.y;
	game->map[x + y * game->map_width].base.num_units += 1;
	game->num_units++;
}

// merge sort the list of units, so that they appear in drawing order
void _game_msort_unit_list(GAME *game)
{
	if (game->num_units == 0 || game->num_units == 1)
	{
		// nothing to see here, we are done, units already sorted
		return;
	}

	if (game->unit_list == NULL || game->unit_list->next == NULL)
	{
		// nothing to see here, we are done, units already sorted
		return;
	}

	UNIT_LIST *new_head = game->unit_list, *temp = NULL;
	UNIT_LIST *list[32];
	memset(list, 0, sizeof(UNIT_LIST *) * 32);
	UNIT_LIST *next;
	uint32_t i;

	while (new_head != NULL)
	{
		next = new_head->next;
		new_head->next = NULL;
		for (i = 0; (i < 32) && (list[i] != NULL); ++i)
		{
			new_head = _game_msort_unit_merge(list[i], new_head);
			list[i] = NULL;
		}
		if (i == 32)
		{
			--i;
		}
		list[i] = new_head;
		new_head = next;
	}

	new_head = NULL;
	for (i = 0; i < 32; ++i)
	{
		new_head = _game_msort_unit_merge(list[i], new_head);
	}

	game->unit_list = new_head;
	temp = new_head;

	while (new_head != NULL)
	{
		temp = new_head;
		new_head = new_head->next;
	}

	game->unit_list_end = temp;
}

UNIT_LIST *_game_msort_unit_merge(UNIT_LIST *first, UNIT_LIST *second)
{
	UNIT_LIST *result = NULL;
	UNIT_LIST *result_end = NULL;
	UNIT_LIST *first_follow = first, *second_follow = second;

	while (first_follow != NULL && second_follow != NULL)
	{
		UNIT_LIST *temp = _game_msort_unit_compare(first_follow, second_follow);
		if (temp == NULL)
		{
			break;
		}

		if (temp == first_follow)
		{
			UNIT_LIST *next = first_follow->next;
			if (result == NULL)
			{
				result = first_follow;
				first_follow->prev = NULL;
				first_follow->next = NULL;
				result_end = first_follow;
			}
			else
			{
				result_end->next = first_follow;
				first_follow->prev = result_end;
				first_follow->next = NULL;

				result_end = first_follow;
			}

			first_follow = next;
		}
		else
		{
			UNIT_LIST *next = second_follow->next;
			if (result == NULL)
			{
				result = second_follow;
				second_follow->prev = NULL;
				second_follow->next = NULL;
				result_end = second_follow;
			}
			else
			{
				result_end->next = second_follow;
				second_follow->prev = result_end;
				second_follow->next = NULL;

				result_end = second_follow;
			}

			second_follow = next;
		}
	}

	// either first or second may have content left, empty them
	while (first_follow != NULL)
	{
		UNIT_LIST *next = first_follow->next;
		if (result == NULL)
		{
			result = first_follow;
			first_follow->prev = NULL;
			first_follow->next = NULL;
			result_end = first_follow;
		}
		else
		{
			result_end->next = first_follow;
			first_follow->prev = result_end;
			first_follow->next = NULL;

			result_end = first_follow;
		}

		first_follow = next;
	}

	while (second_follow != NULL)
	{
		UNIT_LIST *next = second_follow->next;
		if (result == NULL)
		{
			result = second_follow;
			second_follow->prev = NULL;
			second_follow->next = NULL;
			result_end = second_follow;
		}
		else
		{
			result_end->next = second_follow;
			second_follow->prev = result_end;
			second_follow->next = NULL;

			result_end = second_follow;
		}

		second_follow = next;
	}

	return result;
}

UNIT_LIST *_game_msort_unit_compare(UNIT_LIST *first, UNIT_LIST *second)
{
	int32_t ax, ay, bx, by;
	double afx, afy, bfx, bfy;

	if (first == NULL)
	{
		return second;
	}
	if (second == NULL)
	{
		return first;
	}

	//sorting criteria
	ax = (int32_t)first->unit.base.x;
	ay = (int32_t)first->unit.base.y;

	bx = (int32_t)second->unit.base.x;
	by = (int32_t)second->unit.base.y;

	// first criteria: which "tile y" is less (render order / topwards)
	if (-ax + ay < -bx + by)
	{
		return first;
	}
	if (-ax + ay > -bx + by)
	{
		return second;
	}

	// second criteria: which "tile x" is less (leftwards)
	if (ax < bx)
	{
		return first;
	}
	if (ax > bx)
	{
		return second;
	}

	// third criteria: which "screen y" is less (topwards)
	afx = first->unit.base.x;
	afy = first->unit.base.y;

	bfx = second->unit.base.x;
	bfy = second->unit.base.y;

	if (-afx + afy < -bfx + bfy)
	{
		return first;
	}
	if (-afx + afy > -bfx + bfy)
	{
		return second;
	}

	// default to this when all else fails
	return first;
}