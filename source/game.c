#include "game.h"
#include "input.h"
#include "render.h"
#include "system.h"
#include "log.h"
#include "menu.h"
#include "unit.h"
#include "pathing.h"
#include "building.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NUM_UNITS (100)

#define SCREEN_SPEED_X (4)
#define SCREEN_SPEED_Y (2)

#define MAP_FILE_VERSION (0)

typedef enum mouse_state
{
	MOUSE_DOWN,
	MOUSE_UP
} MOUSE_STATE;

typedef struct mouse
{
	MOUSE_STATE prev_left;
	MOUSE_STATE left;
	MOUSE_STATE prev_right;
	MOUSE_STATE right;
	int32_t left_x;
	int32_t left_y;
	int32_t right_x;
	int32_t right_y;
	int32_t cur_x;
	int32_t cur_y;
} MOUSE;

struct game
{
	MOUSE mouse;
	int32_t x_offset;
	int32_t y_offset;
	UNIT_LIST *unit_list;
	UNIT_LIST *unit_list_end;
	uint64_t num_units;
	BUILDING_LIST *building_list;
	BUILDING_LIST *building_list_end;
	uint64_t num_buildings;
	MAP *map;
	uint8_t quit;
};

void _game_mouse_button_event_cb(SDL_MouseButtonEvent ev, void *ptr);
void _game_mouse_motion_event_cb(SDL_MouseMotionEvent ev, void *ptr);
void _game_init(GAME *game, STATE *state, INPUT_S *input);
void _game_play(GAME *game, STATE *state);
void _game_post(GAME *game, STATE *state, INPUT_S *input);
void _game_scroll(GAME *game);
void _game_draw_mouse(GAME *game);
void _game_update_mouse(GAME *game);
double _game_get_unit_z(double x, double y, uint8_t corners);
void _game_create_unit(GAME *game, UNIT unit);
void _game_create_unit_gunner(GAME *game, double x, double y);
void _game_unit_path_update(GAME *game);
void _game_pathfind(GAME *game, UNIT *unit, double x, double y);

void _game_create_building(GAME *game, BUILDING building);
void _game_create_building_house(GAME *game, uint32_t x, uint32_t y);

void game_loop(MENU_S *menu)
{
	STATE state = STATE_MENU_MAIN;
	GAME game;

	INPUT_S *input;

	input = input_setup();

	while (input_poll(input) == 0)
	{
		render_begin_frame();
		switch (state)
		{
			case STATE_MENU_MAIN:
			case STATE_MENU_SINGLE_OPTIONS:
			case STATE_MENU_MULTI_OPTIONS:
				menu_display(menu, input, &state);
				break;
			case STATE_GAME_SINGLE_INIT:
			case STATE_GAME_MULTI_INIT:
				_game_init(&game, &state, input);
				break;
			case STATE_GAME_SINGLE_PLAY:
			case STATE_GAME_MULTI_PLAY:
				_game_play(&game, &state);
				break;
			case STATE_GAME_SINGLE_POST:
			case STATE_GAME_MULTI_POST:
				_game_post(&game, &state, input);
				break;
			case STATE_EXIT:
				input_teardown(input);
				return;
		}
		render_end_frame();
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
				game_unit_coords_to_drawing_coords(game, unit_list->unit.base.x, unit_list->unit.base.y, &x, &y);
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
					double x, y;
					game_screen_coords_to_unit_coords(game, game->mouse.cur_x, game->mouse.cur_y, &x, &y);
					_game_pathfind(game, &unit_list->unit, x, y);
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

void _game_init(GAME *game, STATE *state, INPUT_S *input)
{
	uint32_t i;

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

	//game->map = map_generate_random(50, 50);

	// load map from file
	//game->map = map_load(3);
	

	// create blank map
	game->map = map_generate_blank(10, 10);
	
	game->x_offset = (int32_t)((map_get_width(game->map) * (TILE_WIDTH / 2) + map_get_height(game->map) * (TILE_WIDTH / 2)) / 2 - (SCREEN_WIDTH / 2));
	game->y_offset = (int32_t)(-1 * (SCREEN_HEIGHT / 2) + (map_get_height(game->map) * (TILE_HEIGHT / 2) - map_get_width(game->map) * (TILE_HEIGHT / 2)) / 2);
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

	for (i = 0; i < 3; i++)
	{
		_game_create_unit_gunner(game, (system_rand() % (map_get_width(game->map) * 10)) / 10.0, (system_rand() % (map_get_height(game->map) * 10)) / 10.0);
	}

	map_set_unit_meshes(game->map);

	_game_create_building_house(game, 2, 2);

	/*
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

void _game_play(GAME *game, STATE *state)
{
	_game_update_mouse(game);
	_game_scroll(game);

	unit_msort_unit_list(&(game->unit_list), &(game->unit_list_end));
	building_msort_building_list(&(game->building_list), &(game->building_list_end));
	_game_unit_path_update(game);

	map_update_units(game->map, game->unit_list);
	map_update_buildings(game->map, game->building_list);
	map_draw(game->map, game->unit_list, game->building_list, game->x_offset, game->y_offset);
	_game_draw_mouse(game);

	if (game->quit)
	{
		game->quit = 0;
		*state = STATE_GAME_SINGLE_POST;
		log_output("game: State change (%s->%s)\n", game_state_to_string(STATE_GAME_SINGLE_PLAY), game_state_to_string(*state));
	}
}

void _game_post(GAME *game, STATE *state, INPUT_S *input)
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

	map_destroy(game->map);
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

		game_screen_coords_to_unit_coords(game, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, &x, &y);

		double orig_x = x;
		double orig_y = y;

		double w = (double)map_get_width(game->map);
		double h = (double)map_get_height(game->map);

		if (x < 0.0)
		{
			clamp = 1;
			x = 0.0;
		}
		if (x > w)
		{
			clamp = 1;
			x = w;
		}
		if (y < 0.0)
		{
			clamp = 1;
			y = 0.0;
		}
		if (y > h)
		{
			clamp = 1;
			y = h;
		}

		if (clamp)
		{
			int32_t screen_x, screen_y;
			int32_t orig_screen_x, orig_screen_y;

			game_unit_coords_to_screen_coords(game, x, y, &screen_x, &screen_y);
			game_unit_coords_to_screen_coords(game, orig_x, orig_y, &orig_screen_x, &orig_screen_y);
			
			int32_t diff_x = screen_x - orig_screen_x;
			int32_t diff_y = screen_y - orig_screen_y;

			game->x_offset += diff_x;
			game->y_offset += diff_y;
		}
	}
}

void _game_draw_mouse(GAME *game)
{
	if (game->mouse.left == MOUSE_DOWN)
	{
		render_rectangle(game->mouse.left_x, game->mouse.left_y, game->mouse.cur_x - game->mouse.left_x + 1, game->mouse.cur_y - game->mouse.left_y + 1, 0x00, 0xFF, 0x00, 0xFF, OUTLINE);
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

void game_unit_coords_to_drawing_coords(GAME *game, double unit_x, double unit_y, int32_t *screen_x, int32_t *screen_y)
{
	if (screen_x == NULL || screen_y == NULL)
	{
		log_output("game: Parameter was NULL: screen_x = %i, screen_y = %i\n", screen_x, screen_y);
		return;
	}
	map_unit_coords_to_drawing_coords(game->map, unit_x, unit_y, screen_x, screen_y);
	*screen_x -= game->x_offset;
	*screen_y -= game->y_offset;
}

void game_unit_coords_to_screen_coords(GAME *game, double unit_x, double unit_y, int32_t *screen_x, int32_t *screen_y)
{
	if (screen_x == NULL || screen_y == NULL)
	{
		log_output("game: Parameter was NULL: screen_x = %i, screen_y = %i\n", screen_x, screen_y);
		return;
	}
	map_unit_coords_to_logical_coords(unit_x, unit_y, screen_x, screen_y);
	*screen_x -= game->x_offset;
	*screen_y -= game->y_offset;
}

void game_screen_coords_to_unit_coords(GAME *game, int32_t screen_x, int32_t screen_y, double *unit_x, double *unit_y)
{
	if (unit_x == NULL || unit_y == NULL)
	{
		log_output("game: Parameter was NULL: unit_x = %i, unit_y = %i\n", unit_x, unit_y);
		return;
	}
	int32_t x_orig = screen_x + game->x_offset;
	int32_t y_orig = screen_y + game->y_offset;
	map_logical_coords_to_unit_coords(x_orig, y_orig, unit_x, unit_y);
}

void _game_create_unit(GAME *game, UNIT unit)
{
	unit_insert(&(game->unit_list), &(game->unit_list_end), unit);
}

void _game_create_unit_gunner(GAME *game, double x, double y)
{
	UNIT unit;

	// --- //
	unit = unit_defaults[TYPE_GUNNER];
	unit.base.x = x;
	unit.base.y = y;
	// --- //

	_game_create_unit(game, unit);
}

void _game_unit_path_update(GAME *game)
{
	UNIT_LIST *head = game->unit_list;
	while (head != NULL)
	{
		if (head->unit.base.path != NULL)
		{
			unit_path_step(&head->unit);
		}
		head = head->next;
	}
}

void _game_pathfind(GAME *game, UNIT *unit, double x, double y)
{
	map_find_path(game->map, unit, x, y);
}

void game_draw_nodes(GAME *game, NODE_MESH *mesh)
{
	uint64_t i, j;
	NODE_MESH *node_mesh = mesh;

	for (i = 0; i < node_mesh->mesh_size; i++)
	{
		int32_t x, y;
		game_unit_coords_to_screen_coords(game, node_mesh->mesh[i].x, node_mesh->mesh[i].y, &x, &y);
		render_circle(x, y, 4, 0xFF, 0x00, 0x00, 1);
		for (j = 0; j < node_mesh->mesh[i].los_nodes_size; j++)
		{
			int32_t x2, y2;
			uint64_t index = node_mesh->mesh[i].los_nodes[j].index;
			game_unit_coords_to_screen_coords(game, node_mesh->mesh[index].x, node_mesh->mesh[index].y, &x2, &y2);
			render_line(x, y, x2, y2, 0xFF, 0x00, 0x00, 1);
		}
		char str[5];
		sprintf_s(str, 5, "%llu", i);
		render_draw_text(x, y, str, 2, 0x7F, 0x00, 0x00, 0xFF, ALIGN_CENTER, ALIGN_CENTER, QUALITY_BEST, 1);
	}
}

void _game_create_building(GAME *game, BUILDING building)
{
	building_insert(&(game->building_list), &(game->building_list_end), building);
}

void _game_create_building_house(GAME *game, uint32_t x, uint32_t y)
{
	BUILDING building;

	building = building_defaults[TYPE_HOUSE];
	building.base.x = x;
	building.base.y = y;

	_game_create_building(game, building);

	map_add_node(game->map, x, y, 1, 1);
}