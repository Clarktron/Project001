#ifndef MAP_H
#define MAP_H

#include <stdint.h>

#include "render.h"
#include "unit.h"

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
} TILE_BASE;

typedef union tile
{
	TILE_TYPE type;
	TILE_BASE base;
} TILE;

typedef struct map_node
{
	uint32_t *los_nodes;
	uint32_t num_los_nodes;
	double x;
	double y;
} MAP_NODE;

typedef struct map MAP;

void map_draw_tile(TILE tile, int32_t x, int32_t y, uint64_t index);
MAP *map_generate_random(uint64_t w, uint64_t h);
MAP *map_generate_blank(uint64_t w, uint64_t h);
MAP *map_load(uint64_t index);
void map_destroy(MAP *map);
uint64_t map_get_width(MAP *map);
uint64_t map_get_height(MAP *map);
uint32_t map_get_elevation(MAP *map, uint64_t x, uint64_t y);
uint8_t map_get_corners(MAP *map, uint64_t x, uint64_t y);
TILE map_get_tile(MAP *map, uint64_t x, uint64_t y);
void map_draw(MAP *map, UNIT_LIST *list, int32_t x_off, int32_t y_off);
void map_unit_coords_to_drawing_coords(MAP *map, double unit_x, double unit_y, int32_t *screen_x, int32_t *screen_y);
void map_unit_coords_to_logical_coords(double unit_x, double unit_y, int32_t *screen_x, int32_t *screen_y);
void map_logical_coords_to_unit_coords(int32_t screen_x, int32_t screen_y, double *unit_x, double *unit_y);
double map_get_unit_z(double x, double y, uint8_t corners);
void map_update_units(MAP *map, UNIT_LIST *unit_list);
uint8_t map_unit_is_on_tile(MAP *map, double unit_x, double unit_y, uint64_t tile_x, uint64_t tile_y, double radius);
void map_set_unit_meshes(MAP *map);
void map_find_path(MAP *map, UNIT *unit, double x, double y);

#endif