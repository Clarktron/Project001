#ifndef GAME_H
#define GAME_H

#include "state.h"
#include "menu.h"
#include "render.h"
#include "unit.h"

#include <stdint.h>

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
	uint64_t num_units;
} TILE_BASE;

typedef union tile
{
	TILE_TYPE type;
	TILE_BASE base;
} TILE;


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

char *game_state_to_string(STATE state);
void game_loop(RENDER_S *render, MENU_S *menu);

#endif