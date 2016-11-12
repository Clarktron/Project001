#ifndef PATHING_H
#define PATHING_H

#include "map.h"
#include "unit.h"
#include "game.h"

#include <stdint.h>

#define NODE_OFFSET_CONTACT (1)

typedef struct connector
{
	uint64_t index;
	DIM dist;
} CONNECTOR;

typedef struct node
{
	CONNECTOR *los_nodes;
	uint64_t los_nodes_size;
	uint64_t los_nodes_alloc;
	DIM x;
	DIM y;
	DIM f_score;
	DIM g_score;
} NODE;

typedef struct wall_grid
{
	uint8_t *wall_grid;
	DIM_GRAN wall_w;
	DIM_GRAN wall_h;
} WALL_GRID;

typedef struct node_mesh
{
	NODE *mesh;
	DIM_GRAN w;
	DIM_GRAN h;
	uint64_t mesh_size;
	uint64_t mesh_alloc;
} NODE_MESH;

NODE_MESH *pathing_create_mesh(WALL_GRID *grid, DIM_GRAN w, DIM_GRAN h, DIM size);
NODE_MESH *pathing_create_disconnected_mesh(WALL_GRID *grid, DIM_GRAN w, DIM_GRAN h, DIM size);
void pathing_destroy_mesh(NODE_MESH *mesh);
void pathing_find_path(NODE_MESH *mesh, WALL_GRID *grid, UNIT *unit, DIM x_dest, DIM y_dest);
//uint64_t pathing_node_mesh_insert(NODE_MESH *mesh, DIM x, DIM y, DIM size, WALL_GRID *grid);
void pathing_node_mesh_remove_end(NODE_MESH *mesh);
WALL_GRID *pathing_generate_wall_grid(MAP *map);
void pathing_destroy_wall_grid(WALL_GRID *grid);
void pathing_draw_walls(WALL_GRID *grid);
void pathing_draw_nodes(NODE_MESH *mesh);

#endif
