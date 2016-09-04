#ifndef PATHING_H
#define PATHING_H

#include "map.h"
#include "unit.h"
#include "game.h"

#include <stdint.h>

#define NODE_OFFSET_CONTACT (0.00001)

typedef struct connector
{
	uint64_t index;
	double dist;
} CONNECTOR;

typedef struct node
{
	CONNECTOR *los_nodes;
	uint64_t los_nodes_size;
	uint64_t los_nodes_alloc;
	double x;
	double y;
	double f_score;
	double g_score;
} NODE;

typedef struct wall_grid
{
	uint8_t *wall_grid;
	uint64_t wall_w;
	uint64_t wall_h;
} WALL_GRID;

typedef struct node_mesh
{
	NODE *mesh;
	uint64_t mesh_size;
	uint64_t mesh_alloc;
} NODE_MESH;

NODE_MESH *pathing_create_mesh(WALL_GRID *grid, double size);
NODE_MESH *pathing_create_disconnected_mesh(WALL_GRID *grid, double size);
void pathing_destroy_mesh(NODE_MESH *mesh);
void pathing_find_path(NODE_MESH *mesh, WALL_GRID *grid, UNIT *unit, double x_dest, double y_dest);
uint64_t pathing_node_mesh_insert(NODE_MESH *mesh, double x, double y, double size, WALL_GRID *grid);
void pathing_node_mesh_remove(NODE_MESH *mesh, uint64_t index);
WALL_GRID *pathing_generate_wall_grid(MAP *map);
void pathing_destroy_wall_grid(WALL_GRID *grid);
void pathing_draw_walls(WALL_GRID *grid);
void pathing_draw_nodes(NODE_MESH *mesh);

#endif
