#include "pathing.h"
#include "map.h"
#include "log.h"
#include "unit.h"
#include "render.h"
#include "game.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

#define INITIAL_MESH_NODES (1)
#define INITIAL_CONNECTING_NODES (1)

void _pathing_add_connecting_node(NODE *node, uint64_t index, double dist);
void _pathing_generate_mesh(NODE_MESH *mesh, double size, WALL_GRID *grid);
void _pathing_connect_node(NODE_MESH *mesh, double size, uint64_t a, uint64_t b, WALL_GRID *grid);
void _pathing_connect_mesh(NODE_MESH *mesh, double size, WALL_GRID *grid);
uint8_t _pathing_los_check(double ax1, double ay1, double ax2, double ay2, double bx1, double by1, double bx2, double by2);
uint8_t _pathing_corners_match(MAP *map, uint64_t x, uint64_t y, uint8_t up);
uint8_t _pathing_point_on_segment(double x1, double y1, double x2, double y2, double x3, double y3);
uint8_t _pathing_triplet_orientation(double x1, double y1, double x2, double y2, double x3, double y3);
double _pathing_get_distance(double x1, double y1, double x2, double y2);
void _pathing_add_node_to_heap(NODE_MESH mesh, uint64_t **heap, uint64_t *heap_size, uint64_t *heap_alloc, uint64_t node);
uint64_t _pathing_remove_node_from_heap(NODE_MESH *mesh, uint64_t **heap, uint64_t *heap_size);
uint64_t _pathing_node_mesh_insert_quick(NODE_MESH *mesh, double x, double y);

uint64_t pathing_node_mesh_insert(NODE_MESH *mesh, double x, double y, double size, WALL_GRID *grid)
{
	uint64_t i;
	uint64_t index = _pathing_node_mesh_insert_quick(mesh, x, y);

	for (i = 0; i < mesh->mesh_size; ++i)
	{
		if (i != index)
		{
			_pathing_connect_node(mesh, size, index, i, grid);
			_pathing_connect_node(mesh, size, i, index, grid);
		}
	}

	return index;
}

void pathing_node_mesh_remove(NODE_MESH *mesh, uint64_t index)
{
	uint64_t i, j, k;

	for (i = 0; i < mesh->mesh[index].los_nodes_size; ++i)
	{
		if (i != index)
		{
			uint64_t connected = mesh->mesh[index].los_nodes[i].index;

			for (j = 0; j < mesh->mesh[connected].los_nodes_size; ++j)
			{
				if (mesh->mesh[connected].los_nodes[j].index == index)
				{
					CONNECTOR *connections = NULL;
					uint64_t count = 0;
					mesh->mesh[connected].los_nodes_size--;
					connections = (CONNECTOR *)malloc(sizeof(CONNECTOR) * mesh->mesh[connected].los_nodes_size);
					if (connections == NULL)
					{
						log_output("pathing: Unable to allocate memory\n");
						return;
					}
					memcpy(connections, mesh->mesh[connected].los_nodes, sizeof(CONNECTOR) * mesh->mesh[connected].los_nodes_size);

					memset(mesh->mesh[connected].los_nodes, 0, mesh->mesh[connected].los_nodes_size);

					for (k = 0; k < mesh->mesh[connected].los_nodes_size; ++k)
					{
						uint64_t temp_index = connections[k].index;
						if (temp_index != index)
						{
							mesh->mesh[connected].los_nodes[count].dist = connections[k].dist;
							mesh->mesh[connected].los_nodes[count].index = temp_index;
							++count;
						}
					}

					mesh->mesh[connected].los_nodes[mesh->mesh[connected].los_nodes_size].dist = -1.0;
					mesh->mesh[connected].los_nodes[mesh->mesh[connected].los_nodes_size].index = 0;

					free(connections);
					break;
				}
			}
		}
	}
}

uint64_t _pathing_node_mesh_insert_quick(NODE_MESH *mesh, double x, double y)
{
	uint64_t index;
	if (mesh->mesh == NULL)
	{
		mesh->mesh = (NODE *)malloc(sizeof(NODE) * INITIAL_MESH_NODES);
		if (mesh->mesh == NULL)
		{
			log_output("pathing: Unable to allocate memory\n");
			return 0;
		}
		memset(mesh->mesh, 0, sizeof(NODE) * INITIAL_MESH_NODES);
		mesh->mesh_alloc = INITIAL_MESH_NODES;
	}
	else
	{
		if (mesh->mesh_alloc == mesh->mesh_size)
		{
			uint64_t mesh_alloc = mesh->mesh_alloc;
			NODE *temp = NULL;
			mesh->mesh_alloc *= 2;
			temp = (NODE *)realloc(mesh->mesh, sizeof(NODE) * mesh->mesh_alloc);
			if (temp == NULL)
			{
				log_output("pathing: Unable to allocate memory\n");
				free(mesh->mesh);
				return 0;
			}
			mesh->mesh = temp;
			memset(mesh->mesh + mesh_alloc, 0, mesh->mesh_alloc - mesh_alloc);
		}
	}
	index = mesh->mesh_size;
	mesh->mesh[index].x = x;
	mesh->mesh[index].y = y;
	mesh->mesh[index].los_nodes = NULL;
	mesh->mesh[index].los_nodes_size = 0;
	mesh->mesh[index].los_nodes_alloc = 0;
	mesh->mesh[index].f_score = -1.0;
	mesh->mesh[index].g_score = -1.0;
	mesh->mesh_size++;

	return index;
}

void _pathing_add_connecting_node(NODE *node, uint64_t index, double dist)
{
	if (node->los_nodes == NULL)
	{
		node->los_nodes = (CONNECTOR *)malloc(sizeof(CONNECTOR) * INITIAL_CONNECTING_NODES);
		if (node->los_nodes == NULL)
		{
			log_output("pathing: Unable to allocate memory\n");
			return;
		}
		memset(node->los_nodes, 0, sizeof(CONNECTOR) * INITIAL_CONNECTING_NODES);
		node->los_nodes_alloc = INITIAL_CONNECTING_NODES;
	}
	else
	{
		if (node->los_nodes_alloc == node->los_nodes_size)
		{
			uint64_t nodes_alloc = node->los_nodes_alloc;
			CONNECTOR *temp = NULL;
			node->los_nodes_alloc *= 2;
			temp = (CONNECTOR *)realloc(node->los_nodes, sizeof(CONNECTOR) * node->los_nodes_alloc);
			if (temp == NULL)
			{
				log_output("pathing: Unable to allocate memory\n");
				free(node->los_nodes);
				return;
			}
			node->los_nodes = temp;
			memset(node->los_nodes + nodes_alloc, 0, node->los_nodes_alloc - nodes_alloc);
		}
	}
	node->los_nodes[node->los_nodes_size].index = index;
	node->los_nodes[node->los_nodes_size].dist = dist;
	node->los_nodes_size++;
}

WALL_GRID *pathing_generate_wall_grid(MAP *map)
{
	uint64_t i, j;
	WALL_GRID *grid;

	grid = malloc(sizeof(WALL_GRID));
	if (grid == NULL)
	{
		log_output("game: Unable to allocate memory\n");
		return NULL;
	}

	// create walls
	// 0x01 is left wall of current tile
	// 0x02 is top wall of current tile
	//         |
	//        top
	//         |
	//         |   right
	// --left--+---------
	//         |XXXXXXXXX
	//         |XXXXXXXXX
	//   bottom|XXXXXXXXX
	//         |XXXXXXXXX
	grid->wall_w = map_get_width(map) + 1;
	grid->wall_h = map_get_height(map) + 1;
	grid->wall_grid = malloc(sizeof(uint8_t) * grid->wall_w * grid->wall_h);
	if (grid->wall_grid == NULL)
	{
		log_output("game: Unable to allocate memory\n");
		return NULL;
	}

	memset(grid->wall_grid, 0, sizeof(uint8_t) * grid->wall_w * grid->wall_h);
	for (j = 0; j < grid->wall_h; j++)
	{
		for (i = 0; i < grid->wall_w; i++)
		{
			if ((i == 0 || i == grid->wall_w - 1) && j < grid->wall_h - 1)
			{
				grid->wall_grid[i + j * grid->wall_w] |= 0x01;
			}
			else if (i > 0 && i < grid->wall_w - 1 && j < grid->wall_h - 1 && !_pathing_corners_match(map, i, j, 0))
			{
				grid->wall_grid[i + j * grid->wall_w] |= 0x01;
			}

			if ((j == 0 || j == grid->wall_h - 1) && i < grid->wall_w - 1)
			{
				grid->wall_grid[i + j * grid->wall_w] |= 0x02;
			}
			else if (j > 0 && j < grid->wall_h - 1 && i < grid->wall_h - 1 && !_pathing_corners_match(map, i, j, 1))
			{
				grid->wall_grid[i + j * grid->wall_w] |= 0x02;
			}
		}
	}
	return grid;
}

void pathing_destroy_wall_grid(WALL_GRID *grid)
{
	if (grid != NULL)
	{
		if (grid->wall_grid != NULL)
		{
			free(grid->wall_grid);
		}
		free(grid);
	}
}

void _pathing_generate_mesh(NODE_MESH *mesh, double size, WALL_GRID *grid)
{
	uint64_t i, j;
	uint8_t *map_border = grid->wall_grid;
	uint64_t w_border = grid->wall_w;
	uint64_t h_border = grid->wall_h;
	uint64_t inserts = 0;

	// nodify corners
	for (j = 0; j < h_border - 1; j++)
	{
		for (i = 0; i < w_border - 1; i++)
		{
			uint8_t walls = 0;

			if (i > 0 && map_border[i - 1 + j * w_border] & 0x02)
			{
				// left wall
				walls |= 0x01;
			}

			if (j > 0 && map_border[i + (j - 1) * w_border] & 0x01)
			{
				// top wall
				walls |= 0x02;
			}

			if (map_border[i + j * w_border] & 0x02)
			{
				// right wall
				walls |= 0x04;
			}

			if (map_border[i + j * w_border] & 0x01)
			{
				// bottom wall
				walls |= 0x08;
			}

			double x_left, y_up, x_right, y_down;
			// 0.00001 is so that we don't get stuck in walls sometimes
			x_left = i - size - NODE_OFFSET_CONTACT;
			y_up = j - size - NODE_OFFSET_CONTACT;
			x_right = i + size + NODE_OFFSET_CONTACT;
			y_down = j + size + NODE_OFFSET_CONTACT;
			//x_left = i - unit.base.size;
			//y_up = j - unit.base.size;
			//x_right = i + unit.base.size;
			//y_down = j + unit.base.size;

			switch (walls)
			{
				case 0x0C:
					// nodify left corner
					if (i > 0 && j > 0)
					{
						_pathing_node_mesh_insert_quick(mesh, x_left, y_up);
					}
					inserts++;
					break;
				case 0x09:
					// nodify top corner
					_pathing_node_mesh_insert_quick(mesh, x_right, y_up);
					inserts++;
					break;
				case 0x03:
					// nodify right corner
					_pathing_node_mesh_insert_quick(mesh, x_right, y_down);
					inserts++;
					break;
				case 0x06:
					// nodify bottom corner
					_pathing_node_mesh_insert_quick(mesh, x_left, y_down);
					inserts++;
					break;
				case 0x08:
					// nodify left corner
					_pathing_node_mesh_insert_quick(mesh, x_left, y_up);
					inserts++;
					// nodify top corner
					_pathing_node_mesh_insert_quick(mesh, x_right, y_up);
					inserts++;
					break;
				case 0x01:
					// nodify top corner
					_pathing_node_mesh_insert_quick(mesh, x_right, y_up);
					inserts++;
					// nodify right corner
					_pathing_node_mesh_insert_quick(mesh, x_right, y_down);
					inserts++;
					break;
				case 0x02:
					// nodify right corner
					_pathing_node_mesh_insert_quick(mesh, x_right, y_down);
					inserts++;
					// nodify bottom corner
					_pathing_node_mesh_insert_quick(mesh, x_left, y_down);
					inserts++;
					break;
				case 0x04:
					// nodify left corner
					_pathing_node_mesh_insert_quick(mesh, x_left, y_up);
					inserts++;
					// nodify bottom corner
					_pathing_node_mesh_insert_quick(mesh, x_left, y_down);
					inserts++;
					break;
				case 0x00:
				case 0x05:
				case 0x07:
				case 0x0A:
				case 0x0B:
				case 0x0D:
				case 0x0E:
				case 0x0F:
				default:
					break;
			}
		}
	}
}

void _pathing_connect_node(NODE_MESH *mesh, double size, uint64_t a, uint64_t b, WALL_GRID *grid)
{
	uint64_t k, l;
	uint8_t *wall_grid = grid->wall_grid;
	uint64_t wall_w = grid->wall_w;
	double x_a = mesh->mesh[a].x;
	double y_a = mesh->mesh[a].y;

	double x_b = mesh->mesh[b].x;
	double y_b = mesh->mesh[b].y;

	int32_t x_start = max(min((int32_t)mesh->mesh[a].x, (int32_t)mesh->mesh[b].x), 1) - 1;
	int32_t y_start = max(min((int32_t)mesh->mesh[a].y, (int32_t)mesh->mesh[b].y), 1) - 1;
	int32_t x_end = max((int32_t)mesh->mesh[a].x, (int32_t)mesh->mesh[b].x) + 1;
	int32_t y_end = max((int32_t)mesh->mesh[a].y, (int32_t)mesh->mesh[b].y) + 1;

	uint8_t intersect = 0;

	for (l = y_start; l < y_end; l++)
	{
		for (k = x_start; k < x_end; k++)
		{
			// top check
			if (wall_grid[k + l * wall_w] & 0x02)
			{
				double x1 = k - size;
				double x2 = k + 1 + size;
				double y1 = l - size;
				double y2 = l + size;

				if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x2, y1) != 0)
				{
					intersect = 1;
					break;
				}
				else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y2, x2, y2) != 0)
				{
					intersect = 1;
					break;
				}
				else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x1, y2) != 0)
				{
					intersect = 1;
					break;
				}
				else if (_pathing_los_check(x_a, y_a, x_b, y_b, x2, y1, x2, y2) != 0)
				{
					intersect = 1;
					break;
				}
			}

			// left check
			if (wall_grid[k + l * wall_w] & 0x01)
			{
				double x1 = k - size;
				double x2 = k + size;
				double y1 = l - size;
				double y2 = l + 1 + size;

				if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x2, y1) != 0)
				{
					intersect = 1;
					break;
				}
				else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y2, x2, y2) != 0)
				{
					intersect = 1;
					break;
				}
				else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x1, y2) != 0)
				{
					intersect = 1;
					break;
				}
				else if (_pathing_los_check(x_a, y_a, x_b, y_b, x2, y1, x2, y2) != 0)
				{
					intersect = 1;
					break;
				}
			}
		}

		if (intersect)
		{
			break;
		}
	}

	if (!intersect)
	{
		_pathing_add_connecting_node(&mesh->mesh[a], b, _pathing_get_distance(x_a, y_a, x_b, y_b));
	}
}

void _pathing_connect_mesh(NODE_MESH *mesh, double size, WALL_GRID *grid)
{
	uint64_t i, j;
	// Connect the mesh
	for (i = 0; i < mesh->mesh_size; i++)
	{
		for (j = 0; j < mesh->mesh_size; j++)
		{
			if (i != j)
			{
				_pathing_connect_node(mesh, size, i, j, grid);
			}
		}
	}
}

uint8_t _pathing_los_check(double ax1, double ay1, double ax2, double ay2, double bx1, double by1, double bx2, double by2)
{
	uint8_t o1 = _pathing_triplet_orientation(ax1, ay1, ax2, ay2, bx1, by1);
	uint8_t o2 = _pathing_triplet_orientation(ax1, ay1, ax2, ay2, bx2, by2);
	uint8_t o3 = _pathing_triplet_orientation(bx1, by1, bx2, by2, ax1, ay1);
	uint8_t o4 = _pathing_triplet_orientation(bx1, by1, bx2, by2, ax2, ay2);

	if (o1 != o2 && o3 != o4)
	{
		return 1;
	}

	if (o1 == 0 && _pathing_point_on_segment(ax1, ay1, bx1, by1, ax2, ay2))
	{
		return 1;
	}
	if (o2 == 0 && _pathing_point_on_segment(ax1, ay1, bx2, by2, ax2, ay2))
	{
		return 1;
	}
	if (o3 == 0 && _pathing_point_on_segment(bx1, by1, ax1, ay1, bx2, by2))
	{
		return 1;
	}
	if (o4 == 0 && _pathing_point_on_segment(bx1, by1, ax2, ay2, bx2, by2))
	{
		return 1;
	}

	return 0;
}

uint8_t _pathing_corners_match(MAP *map, uint64_t x, uint64_t y, uint8_t up)
{
	uint8_t corners1 = map_get_corners(map, x, y);
	uint8_t corners2;
	uint32_t elev_current = map_get_elevation(map, x, y);
	uint32_t elev_neighbor;
	uint32_t left_point, up_point, right_point, down_point;
	if (up)
	{
		// Up
		elev_neighbor = map_get_elevation(map, x, y - 1);
		corners2 = map_get_corners(map, x, y - 1);

		left_point = ((corners1 & CORNER_L) == CORNER_L);
		left_point += ((corners1 & CORNER_L2) == CORNER_L2) * 2;
		left_point += elev_current;

		up_point = ((corners1 & CORNER_U) == CORNER_U);
		up_point += ((corners1 & CORNER_U2) == CORNER_U2) * 2;
		up_point += elev_current;

		right_point = ((corners2 & CORNER_R) == CORNER_R);
		right_point += ((corners2 & CORNER_R2) == CORNER_R2) * 2;
		right_point += elev_neighbor;

		down_point = ((corners2 & CORNER_D) == CORNER_D);
		down_point += ((corners2 & CORNER_D2) == CORNER_D2) * 2;
		down_point += elev_neighbor;

		if (left_point == down_point && up_point == right_point)
		{
			return 1;
		}
	}
	else
	{
		// Left
		elev_neighbor = map_get_elevation(map, x - 1, y);
		corners2 = map_get_corners(map, x - 1, y);

		left_point = ((corners1 & CORNER_L) == CORNER_L);
		left_point += ((corners1 & CORNER_L2) == CORNER_L2) * 2;
		left_point += elev_current;

		up_point = ((corners2 & CORNER_U) == CORNER_U);
		up_point += ((corners2 & CORNER_U2) == CORNER_U2) * 2;
		up_point += elev_neighbor;

		right_point = ((corners2 & CORNER_R) == CORNER_R);
		right_point += ((corners2 & CORNER_R2) == CORNER_R2) * 2;
		right_point += elev_neighbor;

		down_point = ((corners1 & CORNER_D) == CORNER_D);
		down_point += ((corners1 & CORNER_D2) == CORNER_D2) * 2;
		down_point += elev_current;

		if (left_point == up_point && right_point == down_point)
		{
			return 1;
		}
	}
	return 0;
}

uint8_t _pathing_point_on_segment(double x1, double y1, double x2, double y2, double x3, double y3)
{
	if (x2 <= max(x1, x3) && x2 >= min(x1, x3) && y2 <= max(y1, y3) && y2 >= min(y1, y3))
	{
		return 1;
	}
	return 0;
}

uint8_t _pathing_triplet_orientation(double x1, double y1, double x2, double y2, double x3, double y3)
{
	double val = (y2 - y1) * (x3 - x2) - (x2 - x1) * (y3 - y2);

	if (val == 0)
	{
		return 0;
	}

	return (val > 0) ? 1 : 2;
}

double _pathing_get_distance(double x1, double y1, double x2, double y2)
{
	double x = fabs(x1 - x2);
	double y = fabs(y1 - y2);
	return sqrt(x * x + y * y);
}

NODE_MESH *pathing_create_mesh(WALL_GRID *grid, double size)
{
	NODE_MESH *mesh = NULL;

	// Generate the mesh
	mesh = (NODE_MESH *)malloc(sizeof(NODE_MESH));
	if (mesh == NULL)
	{
		log_output("pathing: Unable to allocate memory\n");
		return NULL;
	}
	memset(mesh, 0, sizeof(NODE_MESH));

	//game_draw_walls(game, wall_grid, wall_w, wall_h);
	// Insert the rest of the nodes based on the map geometry
	_pathing_generate_mesh(mesh, size, grid);

	// Connect the nodes that have LOS to each other
	_pathing_connect_mesh(mesh, size, grid);
	//game_draw_nodes(game, mesh);

	return mesh;
}

void pathing_destroy_mesh(NODE_MESH *mesh)
{
	uint64_t i;
	for (i = 0; i < mesh->mesh_size; i++)
	{
		free(mesh->mesh[i].los_nodes);
	}
	free(mesh->mesh);
	free(mesh);
}

void _pathing_add_node_to_heap(NODE_MESH mesh, uint64_t **heap, uint64_t *heap_size, uint64_t *heap_alloc, uint64_t node)
{
	uint64_t i;
	if (*heap == NULL)
	{
		*heap = (uint64_t *)malloc(sizeof(uint64_t));
		if (*heap == NULL)
		{
			log_output("pathing: Unable to allocate memory\n");
			return;
		}
		memset(*heap, 0, sizeof(uint64_t));
		*heap_alloc = 1;
	}
	else
	{
		if (*heap_alloc == *heap_size)
		{
			uint64_t alloc = *heap_alloc;
			uint64_t *temp = NULL;
			*heap_alloc *= 2;
			temp = (uint64_t *)realloc(*heap, sizeof(uint64_t) * *heap_alloc);
			if (temp == NULL)
			{
				log_output("pathing: Unable to allocate memory\n");
				free(*heap);
				return;
			}
			*heap = temp;
			memset(*heap + alloc, 0, *heap_alloc - alloc);
		}
	}
	(*heap)[*heap_size] = node;
	*heap_size += 1;
	
	if (*heap_size == 1)
	{
		return;
	}

	i = *heap_size;

	while (i > 1)
	{
		uint64_t current = i - 1;
		uint64_t parent = i / 2 - 1;
		if (mesh.mesh[(*heap)[current]].f_score < mesh.mesh[(*heap)[parent]].f_score)
		{
			//swap!
			uint64_t temp = (*heap)[parent];
			(*heap)[parent] = (*heap)[current];
			(*heap)[current] = temp;
		}
		else
		{
			break;
		}
		i /= 2;
	}
}

uint64_t _pathing_remove_node_from_heap(NODE_MESH *mesh, uint64_t **heap, uint64_t *heap_size)
{
	uint64_t value = 0;
	uint64_t i = 1;

	if (*heap_size == 1)
	{
		*heap_size = 0;
		value = (*heap)[0];
		free(*heap);
		*heap = NULL;
		return value;
	}
	else if (*heap_size == 0)
	{
		return 0;
	}

	value = (*heap)[0];
	(*heap)[0] = (*heap)[*heap_size - 1];
	(*heap)[*heap_size - 1] = 0;
	*heap_size -= 1;

	while (i < *heap_size + 1)
	{
		uint64_t child_a = i * 2 - 1;
		uint64_t child_b = i * 2;
		// if our children are not present
		if (child_a >= *heap_size)
		{
			break;
		}

		// if the second child is not present, or the first child is legitimately better
		if (child_b >= *heap_size || mesh->mesh[(*heap)[child_a]].f_score < mesh->mesh[(*heap)[child_b]].f_score)
		{
			if (mesh->mesh[(*heap)[i - 1]].f_score > mesh->mesh[(*heap)[child_a]].f_score)
			{
				//swap with child_a
				uint64_t temp = (*heap)[i - 1];
				(*heap)[i - 1] = (*heap)[child_a];
				(*heap)[child_a] = temp;
			}
			i *= 2;
		}
		else if (mesh->mesh[(*heap)[i - 1]].f_score > mesh->mesh[(*heap)[child_b]].f_score)
		{
			//swap with child_b
			uint64_t temp = (*heap)[i - 1];
			(*heap)[i - 1] = (*heap)[child_b];
			(*heap)[child_b] = temp;

			i *= 2;
			++i;
		}
		else
		{
			break;
		}
	}

	return value;
}

void pathing_find_path(NODE_MESH *mesh, UNIT *unit, double x_dest, double y_dest)
{
	uint64_t *open_nodes = NULL;
	uint64_t open_nodes_size = 0;
	uint64_t open_nodes_alloc = 0;
	uint64_t *closed_nodes = NULL;
	uint64_t closed_nodes_size = 0;
	uint64_t closed_nodes_alloc = 0;
	uint64_t *came_from = NULL;
	uint64_t start = 0;
	uint64_t end = 0;
	uint64_t i, j;

	came_from = malloc(sizeof(uint64_t) * mesh->mesh_size);
	if (came_from == NULL)
	{
		log_output("pathing: Unable to allocate memory\n");
		return;
	}
	memset(came_from, 0xFFFFFFFF, sizeof(uint64_t) * mesh->mesh_size);

	for (i = 0; i < mesh->mesh_size; i++)
	{
		if (fabs(unit->base.x - mesh->mesh[i].x) < 0.00001 && fabs(unit->base.y - mesh->mesh[i].y) < 0.00001)
		{
			start = i;
			break;
		}
	}
	for (i = 0; i < mesh->mesh_size; i++)
	{
		if (fabs(x_dest - mesh->mesh[i].x) < 0.00001 && fabs(y_dest - mesh->mesh[i].y) < 0.00001)
		{
			end = i;
			break;
		}
	}

	_pathing_add_node_to_heap(*mesh, &open_nodes, &open_nodes_size, &open_nodes_alloc, start);
	mesh->mesh[start].g_score = 0.0;
	mesh->mesh[start].f_score = _pathing_get_distance(mesh->mesh[start].x, mesh->mesh[start].y, mesh->mesh[end].x, mesh->mesh[end].y);

	while (open_nodes != NULL)
	{
		uint64_t current = _pathing_remove_node_from_heap(mesh, &open_nodes, &open_nodes_size);
		if (current == end)
		{
			// reconstruct the path and return it
			unit_path_delete(unit);
			unit_path_add_front(unit, mesh->mesh[current].x, mesh->mesh[current].y);
			while (came_from[current] < mesh->mesh_size)
			{
				// previous node is not the start node, take it
				unit_path_add_front(unit, mesh->mesh[came_from[current]].x, mesh->mesh[came_from[current]].y);
				current = came_from[current];
			}
			free(came_from);
			came_from = NULL;
			free(open_nodes);
			open_nodes = NULL;
			free(closed_nodes);
			closed_nodes = NULL;
			return;
		}

		_pathing_add_node_to_heap(*mesh, &closed_nodes, &closed_nodes_size, &closed_nodes_alloc, current);

		for (i = 0; i < mesh->mesh[current].los_nodes_size; i++)
		{
			uint8_t in = 0;
			uint64_t index = mesh->mesh[current].los_nodes[i].index;
			for (j = 0; j < closed_nodes_size; j++)
			{
				if (index == closed_nodes[j])
				{
					in = 1;
					break;
				}
			}
			if (!in)
			{
				in = 0;
				double temp_g;
				for (j = 0; j < open_nodes_size; j++)
				{
					if (index == open_nodes[j])
					{
						in = 1;
						break;
					}
				}
				temp_g = mesh->mesh[current].g_score + mesh->mesh[current].los_nodes[i].dist;
				if (!in)
				{
					came_from[index] = current;
					mesh->mesh[index].g_score = temp_g;
					mesh->mesh[index].f_score = temp_g + _pathing_get_distance(mesh->mesh[index].x, mesh->mesh[index].y, mesh->mesh[end].x, mesh->mesh[end].y);
					_pathing_add_node_to_heap(*mesh, &open_nodes, &open_nodes_size, &open_nodes_alloc, index);
					continue;
				}
				else if (mesh->mesh[index].g_score < 0 || temp_g >= mesh->mesh[index].g_score)
				{
					continue;
				}

				came_from[index] = current;
				mesh->mesh[index].g_score = temp_g;
				mesh->mesh[index].f_score = temp_g + _pathing_get_distance(mesh->mesh[index].x, mesh->mesh[index].y, mesh->mesh[end].x, mesh->mesh[end].y);
			}
		}
	}
	if (open_nodes == NULL)
	{
		// failed
		log_output("pathing: Failed to find path\n");
		if (came_from != NULL)
		{
			free(came_from);
		}
		if (open_nodes != NULL)
		{
			free(open_nodes);
		}
		if (closed_nodes != NULL)
		{
			free(closed_nodes);
		}
		return;
	}
}

void pathing_draw_walls(MAP *map, uint8_t *walls, uint64_t wall_w, uint64_t wall_h, int32_t x_off, int32_t y_off)
{
	uint64_t i, j;

	for (j = 0; j < wall_h; j++)
	{
		for (i = 0; i < wall_w; i++)
		{
			int32_t x1, y1, x2, y2, x3, y3;
			map_unit_coords_to_drawing_coords(map, (double)i, (double)j, &x1, &y1);
			map_unit_coords_to_drawing_coords(map, (double)i + 1, (double)j, &x2, &y2);
			map_unit_coords_to_drawing_coords(map, (double)i, (double)j + 1, &x3, &y3);
			x1 -= x_off;
			x2 -= x_off;
			y1 -= y_off;
			y2 -= y_off;
			if (walls[i + j * wall_h] & 0x01)
			{
				// left
				render_line(x1, y1, x3, y3, 0x00, 0x00, 0xFF, 1);
			}
			if (walls[i + j * wall_h] & 0x02)
			{
				// top
				render_line(x1, y1, x2, y2, 0x00, 0x00, 0xFF, 1);
			}
		}
	}
}
