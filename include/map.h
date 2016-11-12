#ifndef MAP_H
#define MAP_H

#include <stdint.h>

#include "render.h"
#include "unit.h"
#include "building.h"
#include "world.h"

//#define NODE_OFFSET_DRAWING (0.1)

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
	uint64_t num_buildings;
} TILE_BASE;

typedef union tile
{
	TILE_TYPE type;
	TILE_BASE base;
} TILE;

typedef struct map MAP;

void map_draw_tile(TILE tile, int32_t x, int32_t y, uint64_t index);
MAP *map_generate_random(DIM_GRAN w, DIM_GRAN h);
MAP *map_generate_blank(DIM_GRAN w, DIM_GRAN h);
MAP *map_load(uint64_t index);
void map_destroy(MAP *map);
DIM_GRAN map_get_width(MAP *map);
DIM_GRAN map_get_height(MAP *map);
void map_get_offset(MAP *map, int32_t *x_offset, int32_t *y_offset);
void map_set_offset(MAP *map, int32_t x_offset, int32_t y_offset);
uint32_t map_get_elevation(MAP *map, DIM_GRAN x, DIM_GRAN y);
uint8_t map_get_corners(MAP *map, DIM_GRAN x, DIM_GRAN y);
TILE map_get_tile(MAP *map, DIM_GRAN x, DIM_GRAN y);
void map_draw(MAP *map, UNIT_LIST *unit_list, BUILDING_LIST *building_list);
void map_unit_coords_to_drawing_coords(MAP *map, DIM unit_x, DIM unit_y, int32_t *screen_x, int32_t *screen_y);
void map_unit_coords_to_ground_coords(MAP *map, DIM unit_x, DIM unit_y, int32_t *screen_x, int32_t *screen_y);
void map_unit_coords_to_logical_coords(DIM unit_x, DIM unit_y, int32_t *screen_x, int32_t *screen_y);
void map_logical_coords_to_unit_coords(int32_t screen_x, int32_t screen_y, DIM *unit_x, DIM *unit_y);
DIM map_get_unit_z(DIM x, DIM y, uint8_t corners);
void map_update_units(MAP *map, UNIT_LIST *unit_list);
void map_update_buildings(MAP *map, BUILDING_LIST *building_list);
uint8_t map_unit_is_on_tile(MAP *map, DIM unit_x, DIM unit_y, DIM_GRAN tile_x, DIM_GRAN tile_y, DIM radius);
void map_find_path(MAP *map, BUILDING_LIST *building_list, UNIT *unit, DIM x, DIM y);
void map_draw_circle(MAP *map, DIM x, DIM y, DIM r);

#endif