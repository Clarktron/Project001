#include "pathing.h"
#include "map.h"
#include "log.h"
#include "unit.h"
#include "render.h"
#include "game.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define INITIAL_MESH_NODES (10)
#define INITIAL_CONNECTING_NODES (10)
#define INITIAL_LIST_SIZE (10)
#define DRAW_SCALE (30)

void _pathing_add_connecting_node(NODE *node, uint64_t index, DIM dist);
void _pathing_generate_mesh(NODE_MESH *mesh, DIM size, WALL_GRID *grid);
uint8_t _pathing_connect_node(NODE_MESH *mesh, DIM size, uint64_t a, uint64_t b, WALL_GRID *grid);
void _pathing_connect_to_all(NODE_MESH *mesh, WALL_GRID *grid, uint64_t index, DIM size);
//void _pathing_connect_mesh(NODE_MESH *mesh, DIM size, WALL_GRID *grid);
uint8_t _pathing_los_check(DIM ax1, DIM ay1, DIM ax2, DIM ay2, DIM bx1, DIM by1, DIM bx2, DIM by2);
uint8_t _pathing_corners_match(MAP *map, DIM_GRAN x, DIM_GRAN y, uint8_t up);
uint8_t _pathing_point_on_segment(DIM x1, DIM y1, DIM x2, DIM y2, DIM x3, DIM y3);
uint8_t _pathing_triplet_orientation(DIM x1, DIM y1, DIM x2, DIM y2, DIM x3, DIM y3);
DIM _pathing_get_distance(DIM x1, DIM y1, DIM x2, DIM y2);
//void _pathing_add_node_to_heap(NODE_MESH mesh, uint64_t **heap, uint64_t *heap_size, uint64_t *heap_alloc, uint64_t node);
uint64_t _pathing_remove_node_from_heap(NODE_MESH *mesh, uint64_t **heap, uint64_t *heap_size);
uint64_t _pathing_node_mesh_insert(NODE_MESH *mesh, DIM x, DIM y);
uint64_t _pathing_node_mesh_insert_quick(NODE_MESH *mesh, DIM x, DIM y);

/*uint64_t pathing_node_mesh_insert(NODE_MESH *mesh, DIM x, DIM y, DIM size, WALL_GRID *grid)
{
	uint64_t i;
	uint64_t index = _pathing_node_mesh_insert_quick(mesh, x, y);

	mesh->mesh[index].los_nodes = malloc(sizeof(CONNECTOR) * INITIAL_CONNECTING_NODES);
	memset(mesh->mesh[index].los_nodes, 0, sizeof(CONNECTOR) * INITIAL_CONNECTING_NODES);
	mesh->mesh[index].los_nodes_alloc = INITIAL_CONNECTING_NODES;

	// for every node in the mesh
	for (i = 0; i < mesh->mesh_size; ++i)
	{
		// if we aren't the new node
		if (i != index)
		{
			// connect the new node to us
			_pathing_connect_node(mesh, size, index, i, grid);
			// connect us to the new node
			_pathing_connect_node(mesh, size, i, index, grid);
		}
	}

	return index;
}*/

void pathing_node_mesh_remove_end(NODE_MESH *mesh)
{
	uint64_t i, j;
	uint64_t index = mesh->mesh_size - 1;

	// A is the indexed node
	// B is the node referenced
	//  by it's connection
	// 
	//      A's connection (Ax)
	// .--. ------------------> .--.
	// |A |                     |B |
	// '--' <------------------ '--'
	//      B's connection (Bx)

	// for every one of A's connections
	for (i = 0; i < mesh->mesh[index].los_nodes_size; ++i)
	{
		// if Ax isn't pointing at A (always the case?)
		if (mesh->mesh[index].los_nodes[i].index != index)
		{
			// get the index of B
			uint64_t connected = mesh->mesh[index].los_nodes[i].index;

			// for every one of B's connections
			for (j = 0; j < mesh->mesh[connected].los_nodes_size; ++j)
			{
				// if Bx is pointing at us
				if (mesh->mesh[connected].los_nodes[j].index == index)
				{
					// this node is Bn
					uint64_t size = mesh->mesh[connected].los_nodes_size;
					uint64_t last_index;
					DIM last_dist;

					// make B's list one smaller
					mesh->mesh[connected].los_nodes_size--;

					// if Bn isn't the last node in the list
					if (j != size - 1)
					{
						// copy the last connection of B's list into Bn
						last_index = mesh->mesh[connected].los_nodes[size - 1].index;
						last_dist = mesh->mesh[connected].los_nodes[size - 1].dist;

						mesh->mesh[connected].los_nodes[j].index = last_index;
						mesh->mesh[connected].los_nodes[j].dist = last_dist;

						mesh->mesh[connected].los_nodes[size - 1].index = 0;
						mesh->mesh[connected].los_nodes[size - 1].dist = 0;
					}
					else
					{
						// reset the values of Bn
						mesh->mesh[connected].los_nodes[j].index = 0;
						mesh->mesh[connected].los_nodes[j].dist = 0;
					}

					// break, assuming that we only point there once (always the case?)
					break;
				}
			}
		}
	}

	// clear the node
	memset(&(mesh->mesh[index]), 0, sizeof(NODE));
	// make the mesh smaller
	mesh->mesh_size--;
}

uint64_t _pathing_node_mesh_insert(NODE_MESH *mesh, DIM x, DIM y)
{
	uint64_t index;
	if (mesh->mesh == NULL)
	{
		log_output("pathing: Invalid argument(s)\n");
		return 0;
	}
	else
	{
		if (mesh->mesh_alloc == mesh->mesh_size)
		{
			uint64_t mesh_alloc = mesh->mesh_alloc;
			NODE *temp = NULL;

			// Increase the allocation size by an arbitrary amount
			mesh->mesh_alloc += 10;
			temp = (NODE *)realloc(mesh->mesh, sizeof(NODE) * mesh->mesh_alloc);
			if (temp == NULL)
			{
				log_output("pathing: Unable to allocate memory\n");
				free(mesh->mesh);
				return 0;
			}
			mesh->mesh = temp;
			memset(mesh->mesh + mesh_alloc, 0, sizeof(NODE) * (mesh->mesh_alloc - mesh_alloc));
		}
	}
	index = mesh->mesh_size;
	mesh->mesh[index].x = x;
	mesh->mesh[index].y = y;
	mesh->mesh[index].los_nodes = NULL;
	mesh->mesh[index].los_nodes_size = 0;
	mesh->mesh[index].los_nodes_alloc = 0;
	mesh->mesh[index].f_score = -1;
	mesh->mesh[index].g_score = -1;
	mesh->mesh_size++;

	return index;
}

uint64_t _pathing_node_mesh_insert_quick(NODE_MESH *mesh, DIM x, DIM y)
{
	uint64_t index;
	if (mesh->mesh == NULL)
	{
		log_output("pathing: Invalid argument(s)\n");
		return 0;
	}
	index = mesh->mesh_size;
	mesh->mesh[index].x = x;
	mesh->mesh[index].y = y;
	mesh->mesh[index].los_nodes = NULL;
	mesh->mesh[index].los_nodes_size = 0;
	mesh->mesh[index].los_nodes_alloc = 0;
	mesh->mesh[index].f_score = -1;
	mesh->mesh[index].g_score = -1;
	mesh->mesh_size++;

	return index;
}

void _pathing_add_connecting_node(NODE *node, uint64_t index, DIM dist)
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
			memset(node->los_nodes + nodes_alloc, 0, sizeof(CONNECTOR) * (node->los_nodes_alloc - nodes_alloc));
		}
	}
	node->los_nodes[node->los_nodes_size].index = index;
	node->los_nodes[node->los_nodes_size].dist = dist;
	node->los_nodes_size++;
}

WALL_GRID *pathing_generate_wall_grid(MAP *map)
{
	DIM_GRAN i, j;
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

void _pathing_generate_mesh(NODE_MESH *mesh, DIM size, WALL_GRID *grid)
{
	DIM_GRAN i, j;

	mesh->mesh = (NODE *)malloc(sizeof(NODE) * (mesh->w * mesh->h));
	if (mesh == NULL)
	{
		log_output("pathing: Unable to allocate memory\n");
		return;
	}
	memset(mesh->mesh, 0, sizeof(NODE) * (mesh->w * mesh->h));

	mesh->mesh_alloc = mesh->w * mesh->h;

	// nodify the grid
	for (j = 0; j < mesh->h; j++)
	{
		for (i = 0; i < mesh->w; i++)
		{
			/*uint8_t walls = 0;

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
			}*/

			DIM x_left, y_up, x_right, y_down;
			// NODE_OFFSET_CONTACT is so that we don't get stuck in walls sometimes
			x_left = world_dim_builder(i, 0) - size - NODE_OFFSET_CONTACT;
			y_up = world_dim_builder(j, 0) - size - NODE_OFFSET_CONTACT;
			x_right = world_dim_builder(i, 0) + size + NODE_OFFSET_CONTACT;
			y_down = world_dim_builder(j, 0) + size + NODE_OFFSET_CONTACT;

			DIM half = world_dim_builder(1, 0) / 2;

			_pathing_node_mesh_insert_quick(mesh, world_dim_builder(i, 0) + half, world_dim_builder(j, 0) + half);
			// nodify left corner
			//_pathing_node_mesh_insert_quick(mesh, x_left, y_up);
			// nodify top corner
			//_pathing_node_mesh_insert_quick(mesh, x_right, y_up);
			// nodify right corner
			//_pathing_node_mesh_insert_quick(mesh, x_right, y_down);
			// nodify bottom corner
			//_pathing_node_mesh_insert_quick(mesh, x_left, y_down);

			/*switch (walls)
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
			}*/
		}
	}
}

uint8_t _pathing_check_node(WALL_GRID *grid, DIM_GRAN x, DIM_GRAN y, DIM size, DIM x_a, DIM y_a, DIM x_b, DIM y_b)
{
	uint8_t *wall_grid = grid->wall_grid;
	DIM_GRAN wall_w = grid->wall_w;

	// DIM versions of the various DIM_GRAN's we need
	DIM x_dim = world_dim_builder(x, 0);
	DIM y_dim = world_dim_builder(y, 0);
	DIM x_dim_plus = world_dim_builder(x + 1, 0);
	DIM y_dim_plus = world_dim_builder(y + 1, 0);
	DIM x_dim_plus_plus = world_dim_builder(x + 2, 0);
	DIM y_dim_plus_plus = world_dim_builder(y + 2, 0);
	DIM x_dim_minus = world_dim_builder(x - 1, 0);
	DIM y_dim_minus = world_dim_builder(y - 1, 0);

	uint16_t walls = 0;

	if (x >= grid->wall_w - 1 || y >= grid->wall_h - 1)
	{
		return 1;
	}

	walls |= wall_grid[x + y * wall_w];
	walls |= (wall_grid[x + 1 + y * wall_w] & 0x0001) << 2;
	walls |= (wall_grid[x + (y + 1) * wall_w] & 0x0002) << 2;

	if (y > 0)
	{
		walls |= (wall_grid[x + (y - 1) * wall_w] & 0x0001) << 4;
		walls |= (wall_grid[x + 1 + (y - 1) * wall_w] & 0x0001) << 5;
	}

	walls |= (wall_grid[x + (y + 1) * wall_w] & 0x0001) << 6;
	walls |= (wall_grid[x + 1 + (y + 1) * wall_w] & 0x0001) << 7;

	walls |= (wall_grid[x + 1 + y * wall_w] & 0x0002) << 7;
	walls |= (wall_grid[x + 1 + (y + 1) * wall_w] & 0x0002) << 8;

	if (x > 0)
	{
		walls |= (wall_grid[x - 1 + y * wall_w] & 0x0002) << 9;
		walls |= (wall_grid[x - 1 + (y + 1) * wall_w] & 0x0002) << 10;
	}

	// left check
	if (walls & 0x0001)
	{
		DIM x1 = x_dim - size;
		DIM x2 = x_dim + size;
		DIM y1 = y_dim - size;
		DIM y2 = y_dim_plus + size;

		if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x2, y1) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y2, x2, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x1, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x2, y1, x2, y2) != 0)
		{
			return 1;
		}
	}

	// top check
	if (walls & 0x0002)
	{
		DIM x1 = x_dim - size;
		DIM x2 = x_dim_plus + size;
		DIM y1 = y_dim - size;
		DIM y2 = y_dim + size;

		if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x2, y1) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y2, x2, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x1, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x2, y1, x2, y2) != 0)
		{
			return 1;
		}
	}

	// right check
	if (walls & 0x0004)
	{
		DIM x1 = x_dim_plus - size;
		DIM x2 = x_dim_plus + size;
		DIM y1 = y_dim - size;
		DIM y2 = y_dim_plus + size;

		if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x2, y1) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y2, x2, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x1, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x2, y1, x2, y2) != 0)
		{
			return 1;
		}
	}

	// bottom check
	if (walls & 0x0008)
	{
		DIM x1 = x_dim - size;
		DIM x2 = x_dim_plus + size;
		DIM y1 = y_dim_plus - size;
		DIM y2 = y_dim_plus + size;

		if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x2, y1) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y2, x2, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x1, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x2, y1, x2, y2) != 0)
		{
			return 1;
		}
	}

	// top-l check
	if (walls & 0x0010)
	{
		DIM x1 = x_dim - size;
		DIM x2 = x_dim + size;
		DIM y1 = y_dim_minus - size;
		DIM y2 = y_dim + size;

		if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x2, y1) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y2, x2, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x1, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x2, y1, x2, y2) != 0)
		{
			return 1;
		}
	}

	// top-r check
	if (walls & 0x0020)
	{
		DIM x1 = x_dim_plus - size;
		DIM x2 = x_dim_plus + size;
		DIM y1 = y_dim_minus - size;
		DIM y2 = y_dim + size;

		if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x2, y1) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y2, x2, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x1, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x2, y1, x2, y2) != 0)
		{
			return 1;
		}
	}

	// bottom-l check
	if (walls & 0x0040)
	{
		DIM x1 = x_dim - size;
		DIM x2 = x_dim + size;
		DIM y1 = y_dim_plus - size;
		DIM y2 = y_dim_plus_plus + size;

		if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x2, y1) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y2, x2, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x1, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x2, y1, x2, y2) != 0)
		{
			return 1;
		}
	}

	// bottom-r check
	if (walls & 0x0080)
	{
		DIM x1 = x_dim_plus - size;
		DIM x2 = x_dim_plus + size;
		DIM y1 = y_dim_plus - size;
		DIM y2 = y_dim_plus_plus + size;

		if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x2, y1) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y2, x2, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x1, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x2, y1, x2, y2) != 0)
		{
			return 1;
		}
	}

	// right-t check
	if (walls & 0x0100)
	{
		DIM x1 = x_dim_plus - size;
		DIM x2 = x_dim_plus_plus + size;
		DIM y1 = y_dim - size;
		DIM y2 = y_dim + size;

		if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x2, y1) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y2, x2, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x1, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x2, y1, x2, y2) != 0)
		{
			return 1;
		}
	}

	// right-b check
	if (walls & 0x0200)
	{
		DIM x1 = x_dim_plus - size;
		DIM x2 = x_dim_plus_plus + size;
		DIM y1 = y_dim_plus - size;
		DIM y2 = y_dim_plus + size;

		if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x2, y1) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y2, x2, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x1, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x2, y1, x2, y2) != 0)
		{
			return 1;
		}
	}

	// left-t check
	if (walls & 0x0400)
	{
		DIM x1 = x_dim_minus - size;
		DIM x2 = x_dim + size;
		DIM y1 = y_dim - size;
		DIM y2 = y_dim + size;

		if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x2, y1) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y2, x2, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x1, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x2, y1, x2, y2) != 0)
		{
			return 1;
		}
	}

	// left-b check
	if (walls & 0x0800)
	{
		DIM x1 = x_dim_minus - size;
		DIM x2 = x_dim + size;
		DIM y1 = y_dim_plus - size;
		DIM y2 = y_dim_plus + size;

		if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x2, y1) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y2, x2, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x1, y1, x1, y2) != 0)
		{
			return 1;
		}
		else if (_pathing_los_check(x_a, y_a, x_b, y_b, x2, y1, x2, y2) != 0)
		{
			return 1;
		}
	}

	return 0;
}

uint8_t _pathing_connect_node(NODE_MESH *mesh, DIM size, uint64_t a, uint64_t b, WALL_GRID *grid)
{
	DIM_GRAN i, k, l;
	DIM x_a = mesh->mesh[a].x;
	DIM y_a = mesh->mesh[a].y;

	DIM x_b = mesh->mesh[b].x;
	DIM y_b = mesh->mesh[b].y;

	DIM_GRAN x_start, y_start, x_end, y_end;
	x_start = world_get_dim_gran(x_a);
	y_start = world_get_dim_gran(y_a);

	x_end = world_get_dim_gran(x_b);
	y_end = world_get_dim_gran(y_b);

	k = x_start;
	l = y_start;
	
	DIM_GRAN dx = abs(x_end - x_start), dy = abs(y_end - y_start);
	DIM e = world_dim_builder(-1, 0);
	DIM de = 0;

	if (dx > dy)
	{
		uint8_t left = 0;
		uint8_t up = 0;
		if (x_end - x_start < 0)
		{
			left = 1;
		}
		if (y_end - y_start < 0)
		{
			up = 1;
		}

		if (dx != 0)
		{
			de = abs(world_dim_div(dy, dx));
		}

		// We are going to move along the x axis (left to right)
		uint64_t count = abs(x_end - x_start) + 1;
		for (i = 0; i < count; ++i)
		{
			if (_pathing_check_node(grid, k, l, size, x_a, y_a, x_b, y_b))
			{
				/*
				render_line((int32_t)((k - 0.5) * DRAW_SCALE), (int32_t)(l * DRAW_SCALE), (int32_t)((k + 1.5) * DRAW_SCALE), (int32_t)(l * DRAW_SCALE), 0xFF, 0x00, 0x00, 1);
				render_line((int32_t)((k - 0.5) * DRAW_SCALE), (int32_t)((l + 1) * DRAW_SCALE), (int32_t)((k + 1.5) * DRAW_SCALE), (int32_t)((l + 1) * DRAW_SCALE), 0xFF, 0x00, 0x00, 1);
				render_line((int32_t)(k * DRAW_SCALE), (int32_t)((l - 0.5) * DRAW_SCALE), (int32_t)(k * DRAW_SCALE), (int32_t)((l + 1.5) * DRAW_SCALE), 0xFF, 0x00, 0x00, 1);
				render_line((int32_t)((k + 1) * DRAW_SCALE), (int32_t)((l - 0.5) * DRAW_SCALE), (int32_t)((k + 1) * DRAW_SCALE), (int32_t)((l + 1.5) * DRAW_SCALE), 0xFF, 0x00, 0x00, 1);
				*/
				return 1;
			}
			/*
			render_line((int32_t)((k - 0.5) * DRAW_SCALE), (int32_t)(l * DRAW_SCALE), (int32_t)((k + 1.5) * DRAW_SCALE), (int32_t)(l * DRAW_SCALE), 0x00, 0x7F, 0x00, 1);
			render_line((int32_t)((k - 0.5) * DRAW_SCALE), (int32_t)((l + 1) * DRAW_SCALE), (int32_t)((k + 1.5) * DRAW_SCALE), (int32_t)((l + 1) * DRAW_SCALE), 0x00, 0x7F, 0x00, 1);
			render_line((int32_t)(k * DRAW_SCALE), (int32_t)((l - 0.5) * DRAW_SCALE), (int32_t)(k * DRAW_SCALE), (int32_t)((l + 1.5) * DRAW_SCALE), 0x00, 0x7F, 0x00, 1);
			render_line((int32_t)((k + 1) * DRAW_SCALE), (int32_t)((l - 0.5) * DRAW_SCALE), (int32_t)((k + 1) * DRAW_SCALE), (int32_t)((l + 1.5) * DRAW_SCALE), 0x00, 0x7F, 0x00, 1);
			*/
			e += de;
			if (e >= 0.0)
			{
				(up) ? --l : ++l;
				e -= world_dim_builder(1, 0);
			}
			(left) ? --k : ++k;
		}
	}
	else
	{
		uint8_t left = 0;
		uint8_t up = 0;
		if (x_end - x_start < 0)
		{
			left = 1;
		}
		if (y_end - y_start < 0)
		{
			up = 1;
		}

		if (dy != 0)
		{
			de = abs(world_dim_div(dx, dy));
		}

		// We are going to move along the y axis (top to bottom)
		uint64_t count = abs(y_end - y_start) + 1;
		for (i = 0; i < count; ++i)
		{
			if (_pathing_check_node(grid, k, l, size, x_a, y_a, x_b, y_b))
			{
				/*
				render_line((int32_t)((k - 0.5) * DRAW_SCALE), (int32_t)(l * DRAW_SCALE), (int32_t)((k + 1.5) * DRAW_SCALE), (int32_t)(l * DRAW_SCALE), 0xFF, 0x00, 0x00, 1);
				render_line((int32_t)((k - 0.5) * DRAW_SCALE), (int32_t)((l + 1) * DRAW_SCALE), (int32_t)((k + 1.5) * DRAW_SCALE), (int32_t)((l + 1) * DRAW_SCALE), 0xFF, 0x00, 0x00, 1);
				render_line((int32_t)(k * DRAW_SCALE), (int32_t)((l - 0.5) * DRAW_SCALE), (int32_t)(k * DRAW_SCALE), (int32_t)((l + 1.5) * DRAW_SCALE), 0xFF, 0x00, 0x00, 1);
				render_line((int32_t)((k + 1) * DRAW_SCALE), (int32_t)((l - 0.5) * DRAW_SCALE), (int32_t)((k + 1) * DRAW_SCALE), (int32_t)((l + 1.5) * DRAW_SCALE), 0xFF, 0x00, 0x00, 1);
				*/
				return 1;
			}
			/*
			render_line((int32_t)((k - 0.5) * DRAW_SCALE), (int32_t)(l * DRAW_SCALE), (int32_t)((k + 1.5) * DRAW_SCALE), (int32_t)(l * DRAW_SCALE), 0x00, 0x7F, 0x00, 1);
			render_line((int32_t)((k - 0.5) * DRAW_SCALE), (int32_t)((l + 1) * DRAW_SCALE), (int32_t)((k + 1.5) * DRAW_SCALE), (int32_t)((l + 1) * DRAW_SCALE), 0x00, 0x7F, 0x00, 1);
			render_line((int32_t)(k * DRAW_SCALE), (int32_t)((l - 0.5) * DRAW_SCALE), (int32_t)(k * DRAW_SCALE), (int32_t)((l + 1.5) * DRAW_SCALE), 0x00, 0x7F, 0x00, 1);
			render_line((int32_t)((k + 1) * DRAW_SCALE), (int32_t)((l - 0.5) * DRAW_SCALE), (int32_t)((k + 1) * DRAW_SCALE), (int32_t)((l + 1.5) * DRAW_SCALE), 0x00, 0x7F, 0x00, 1);
			*/
			e += de;
			if (e >= 0.0)
			{
				(left) ? --k : ++k;
				e -= world_dim_builder(1, 0);
			}
			(up) ? --l : ++l;
		}
	}

	_pathing_add_connecting_node(&(mesh->mesh[a]), b, _pathing_get_distance(x_a, y_a, x_b, y_b));

	return 0;
}

void _pathing_connect_to_all(NODE_MESH *mesh, WALL_GRID *grid, uint64_t index, DIM size)
{
	uint64_t i;
	if (mesh->mesh[index].los_nodes == NULL)
	{
		for (i = 0; i < mesh->mesh_size; ++i)
		{
			if (i != index)
			{
				if (_pathing_connect_node(mesh, size, index, i, grid) == 0)
				{
					if (mesh->mesh[i].los_nodes != NULL && mesh->mesh[i].los_nodes_size > 0)
					{
						_pathing_connect_node(mesh, size, i, index, grid);
					}
				}
				//render_line((int32_t)(mesh->mesh[current].x * DRAW_SCALE), (int32_t)(mesh->mesh[current].y * DRAW_SCALE), (int32_t)(mesh->mesh[i].x * DRAW_SCALE), (int32_t)(mesh->mesh[i].y * DRAW_SCALE), 0x00, 0x00, 0x00, 1);
			}
		}
	}
}

void _pathing_connect_mesh(NODE_MESH *mesh, DIM size, WALL_GRID *grid)
{
	uint64_t i, j;
	// Connect the mesh
	for (j = 0; j < mesh->h; j++)
	{
		for (i = 0; i < mesh->w; i++)
		{
			//render_line((int32_t)(mesh->mesh[i].x * DRAW_SCALE), (int32_t)(mesh->mesh[i].y * DRAW_SCALE), (int32_t)(mesh->mesh[j].x * DRAW_SCALE), (int32_t)(mesh->mesh[j].y * DRAW_SCALE), 0x00, 0x00, 0x00, 1);
			/*if (_pathing_connect_node(mesh, size, i, j, grid))
			{
				//render_line((int32_t)(mesh->mesh[i].x * DRAW_SCALE), (int32_t)(mesh->mesh[i].y * DRAW_SCALE), (int32_t)(mesh->mesh[j].x * DRAW_SCALE), (int32_t)(mesh->mesh[j].y * DRAW_SCALE), 0xFF, 0x00, 0x00, 1);
			}
			else
			{
				//render_line((int32_t)(mesh->mesh[i].x * DRAW_SCALE), (int32_t)(mesh->mesh[i].y * DRAW_SCALE), (int32_t)(mesh->mesh[j].x * DRAW_SCALE), (int32_t)(mesh->mesh[j].y * DRAW_SCALE), 0x00, 0x00, 0x00, 1);
			}*/

			if (i > 0)
			{
				//connect to left
				_pathing_connect_node(mesh, size, i + j * mesh->w, i - 1 + (j) * mesh->w, grid);
				//_pathing_connect_node(mesh, size, i - 1 + (j) * mesh->w, i + j * mesh->w, grid);

				if (j > 0)
				{
					//connect to top left
					_pathing_connect_node(mesh, size, i + j * mesh->w, i - 1 + (j - 1) * mesh->w, grid);
					//_pathing_connect_node(mesh, size, i - 1 + (j - 1) * mesh->w, i + j * mesh->w, grid);
				}
				if (j < mesh->h - 1)
				{
					//connect to bottom left
					_pathing_connect_node(mesh, size, i + j * mesh->w, i - 1 + (j + 1) * mesh->w, grid);
					//_pathing_connect_node(mesh, size, i - 1 + (j + 1) * mesh->w, i + j * mesh->w, grid);
				}
			}
			if (i < mesh->w - 1)
			{
				//connect to right
				_pathing_connect_node(mesh, size, i + j * mesh->w, i + 1 + (j) * mesh->w, grid);
				//_pathing_connect_node(mesh, size, i + 1 + (j) * mesh->w, i + j * mesh->w, grid);

				if (j > 0)
				{
					//connect to top right
					_pathing_connect_node(mesh, size, i + j * mesh->w, i + 1 + (j - 1) * mesh->w, grid);
					//_pathing_connect_node(mesh, size, i + 1 + (j - 1) * mesh->w, i + j * mesh->w, grid);
				}
				if (j < mesh->h - 1)
				{
					//connect to bottom right
					_pathing_connect_node(mesh, size, i + j * mesh->w, i + 1 + (j + 1) * mesh->w, grid);
					//_pathing_connect_node(mesh, size, i + 1 + (j + 1) * mesh->w, i + j * mesh->w, grid);
				}
			}
			if (j > 0)
			{
				//connect to top
				_pathing_connect_node(mesh, size, i + j * mesh->w, i + (j - 1) * mesh->w, grid);
				//_pathing_connect_node(mesh, size, i + (j - 1) * mesh->w, i + j * mesh->w, grid);
			}
			if (j < mesh->h - 1)
			{
				//connect to bottom
				_pathing_connect_node(mesh, size, i + j * mesh->w, i + (j + 1) * mesh->w, grid);
				//_pathing_connect_node(mesh, size, i + (j + 1) * mesh->w, i + j * mesh->w, grid);
			}
		}
	}
}

uint8_t _pathing_los_check(DIM ax1, DIM ay1, DIM ax2, DIM ay2, DIM bx1, DIM by1, DIM bx2, DIM by2)
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

uint8_t _pathing_corners_match(MAP *map, DIM_GRAN x, DIM_GRAN y, uint8_t up)
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

uint8_t _pathing_point_on_segment(DIM x1, DIM y1, DIM x2, DIM y2, DIM x3, DIM y3)
{
	if (x2 <= max(x1, x3) && x2 >= min(x1, x3) && y2 <= max(y1, y3) && y2 >= min(y1, y3))
	{
		return 1;
	}
	return 0;
}

uint8_t _pathing_triplet_orientation(DIM x1, DIM y1, DIM x2, DIM y2, DIM x3, DIM y3)
{
	DIM val = world_dim_mult(y2 - y1, x3 - x2) - world_dim_mult(x2 - x1, y3 - y2);

	if (val == 0)
	{
		return 0;
	}

	return (val > 0) ? 1 : 2;
}

DIM _pathing_get_distance(DIM x1, DIM y1, DIM x2, DIM y2)
{
	DIM x = abs(x1 - x2);
	DIM y = abs(y1 - y2);
	return world_dim_sqrt(world_dim_mult(x, x) + world_dim_mult(y, y));
}

NODE_MESH *pathing_create_mesh(WALL_GRID *grid, DIM_GRAN w, DIM_GRAN h, DIM size)
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

	mesh->w = w;
	mesh->h = h;

	//game_draw_walls(game, wall_grid, wall_w, wall_h);
	// Insert the rest of the nodes based on the map geometry
	_pathing_generate_mesh(mesh, size, grid);

	// Connect the nodes that have LOS to each other
	_pathing_connect_mesh(mesh, size, grid);
	//game_draw_nodes(game, mesh);

	return mesh;
}

NODE_MESH *pathing_create_disconnected_mesh(WALL_GRID *grid, DIM_GRAN w, DIM_GRAN h, DIM size)
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

	mesh->w = w;
	mesh->h = h;

	// Insert the nodes based on the map geometry
	_pathing_generate_mesh(mesh, size, grid);

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
		*heap = (uint64_t *)malloc(sizeof(uint64_t) * INITIAL_LIST_SIZE);
		if (*heap == NULL)
		{
			log_output("pathing: Unable to allocate memory\n");
			return;
		}
		memset(*heap, 0, sizeof(uint64_t) * INITIAL_LIST_SIZE);
		*heap_alloc = INITIAL_LIST_SIZE;
		*heap_size = 0;
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
			memset(*heap + alloc, 0, sizeof(uint64_t) * (*heap_alloc - alloc));
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

void _pathing_add_node_to_buffer(uint64_t **buffer, uint64_t *buffer_size, uint64_t *buffer_alloc, uint64_t node)
{
	if (*buffer == NULL)
	{
		*buffer = (uint64_t *)malloc(sizeof(uint64_t) * INITIAL_LIST_SIZE);
		if (*buffer == NULL)
		{
			log_output("pathing: Unable to allocate memory\n");
			return;
		}
		memset(*buffer, 0, sizeof(uint64_t) * INITIAL_LIST_SIZE);
		*buffer_alloc = INITIAL_LIST_SIZE;
		*buffer_size = 0;
	}
	else
	{
		if (*buffer_alloc == *buffer_size)
		{
			uint64_t alloc = *buffer_alloc;
			uint64_t *temp = NULL;
			*buffer_alloc *= 2;
			temp = (uint64_t *)realloc(*buffer, sizeof(uint64_t) * *buffer_alloc);
			if (temp == NULL)
			{
				log_output("pathing: Unable to allocate memory\n");
				free(*buffer);
				return;
			}
			*buffer = temp;
			memset(*buffer + alloc, 0, sizeof(uint64_t) * (*buffer_alloc - alloc));
		}
	}

	(*buffer)[*buffer_size] = node;
	*buffer_size += 1;
}

void pathing_find_path(NODE_MESH *mesh, WALL_GRID *grid, UNIT *unit, DIM x_dest, DIM y_dest)
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

	// two more because of the extra start/end nodes
	came_from = malloc(sizeof(uint64_t) * (mesh->mesh_size + 2));
	if (came_from == NULL)
	{
		log_output("pathing: Unable to allocate memory\n");
		return;
	}
	memset(came_from, 0xFFFFFFFF, sizeof(uint64_t) * (mesh->mesh_size + 2));

	start = _pathing_node_mesh_insert(mesh, unit->base.x, unit->base.y);
	end = _pathing_node_mesh_insert(mesh, x_dest, y_dest);

	_pathing_connect_to_all(mesh, grid, start, unit->base.size);
	_pathing_connect_to_all(mesh, grid, end, unit->base.size);

	/*
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
	*/

	_pathing_add_node_to_heap(*mesh, &open_nodes, &open_nodes_size, &open_nodes_alloc, start);
	mesh->mesh[start].g_score = 0;
	mesh->mesh[start].f_score = _pathing_get_distance(mesh->mesh[start].x, mesh->mesh[start].y, mesh->mesh[end].x, mesh->mesh[end].y);

	while (open_nodes != NULL)
	{
		uint64_t current = _pathing_remove_node_from_heap(mesh, &open_nodes, &open_nodes_size);
		
		//render_begin_frame();
		//pathing_draw_walls(grid);
		//pathing_draw_nodes(mesh);

		// if this node has not been connected to the mesh, connect it now
		//_pathing_connect_to_all(mesh, grid, current, unit->base.size);

		//render_begin_frame();
		//pathing_draw_walls(grid);
		//pathing_draw_nodes(mesh);

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

			// get rid of the start/end nodes that were tacked on at the beginning
			pathing_node_mesh_remove_end(mesh);
			pathing_node_mesh_remove_end(mesh);

			return;
		}

		_pathing_add_node_to_buffer(&closed_nodes, &closed_nodes_size, &closed_nodes_alloc, current);

		for (i = 0; i < mesh->mesh[current].los_nodes_size; ++i)
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
				DIM temp_g;
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

	pathing_node_mesh_remove_end(mesh);
	pathing_node_mesh_remove_end(mesh);
}

void pathing_draw_walls(WALL_GRID *grid)
{
	uint64_t i, j;
	uint8_t *walls = grid->wall_grid;
	uint64_t wall_w = grid->wall_w;
	uint64_t wall_h = grid->wall_h;

	for (j = 0; j < wall_h; j++)
	{
		for (i = 0; i < wall_w; i++)
		{
			int32_t x1 = (int32_t)i;
			int32_t x2 = (int32_t)i + 1;
			int32_t y1 = (int32_t)j;
			int32_t y2 = (int32_t)j + 1;
			if (walls[i + j * wall_h] & 0x01)
			{
				// left
				render_line(x1 * DRAW_SCALE, y1 * DRAW_SCALE, x1 * DRAW_SCALE, y2 * DRAW_SCALE, 0x00, 0x00, 0xFF, 1);
			}
			if (walls[i + j * wall_h] & 0x02)
			{
				// top
				render_line(x1 * DRAW_SCALE, y1 * DRAW_SCALE, x2 * DRAW_SCALE, y1 * DRAW_SCALE, 0x00, 0x00, 0xFF, 1);
			}
		}
	}
}

void pathing_draw_nodes(NODE_MESH *mesh)
{
	uint64_t i, j;
	NODE_MESH *node_mesh = (NODE_MESH *)mesh;

	for (i = 0; i < node_mesh->mesh_size; i++)
	{
		DIM x, y;
		x = node_mesh->mesh[i].x;
		y = node_mesh->mesh[i].y;
		render_circle((int32_t)(x * DRAW_SCALE), (int32_t)(y * DRAW_SCALE), 4, 0xFF, 0x00, 0x00, 1);
		for (j = 0; j < node_mesh->mesh[i].los_nodes_size; j++)
		{
			DIM x2, y2;
			uint64_t index = node_mesh->mesh[i].los_nodes[j].index;
			x2 = node_mesh->mesh[index].x;
			y2 = node_mesh->mesh[index].y;
			render_line((int32_t)(x * DRAW_SCALE), (int32_t)(y * DRAW_SCALE), (int32_t)(x2 * DRAW_SCALE), (int32_t)(y2 * DRAW_SCALE), 0xFF, 0x00, 0x00, 1);
			//char str[5];
			//sprintf_s(str, 5, "%llu", index);
			//render_draw_text(x2 * DRAW_SCALE, y2 * DRAW_SCALE, str, 0, 0x00, 0x00, 0x00, 0xFF, ALIGN_CENTER, ALIGN_CENTER, QUALITY_BEST, 1);
		}
		char str[5];
		sprintf_s(str, 5, "%llu", i);
		render_draw_text((int32_t)(x * DRAW_SCALE), (int32_t)(y * DRAW_SCALE), str, 0, 0x00, 0x00, 0x00, 0xFF, ALIGN_CENTER, ALIGN_CENTER, QUALITY_BEST, 1);
	}
}