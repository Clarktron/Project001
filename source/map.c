#include "map.h"
#include "log.h"
#include "render.h"
#include "unit.h"
#include "system.h"
#include "pathing.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define MAP_NAME_LEN (260)

#define SQRT_RIGHT_ANGLE_TRI (0.70710678)

struct map
{
	TILE *map;
	uint64_t map_width;
	uint64_t map_height;
	NODE_MESH *unit_meshes[2];
	WALL_GRID *grid;
};

uint8_t _map_corners_to_index(uint8_t corners);

uint8_t _map_corners_to_index(uint8_t corners)
{
	switch (corners)
	{
		case CORNER_FLAT:
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
}

void map_draw_tile(TILE tile, int32_t x, int32_t y, uint64_t index)
{
	uint32_t i;

	// Draw the dirt foundation, bottom up
	for (i = 0; i < tile.base.elevation; i++)
	{
		render_draw_slope(_map_corners_to_index(CORNER_BASE), x, y - i * TILE_DEPTH);
	}
	// ---

	// Draw the tile surface
	render_draw_slope(_map_corners_to_index(tile.base.corners), x, y - tile.base.elevation * TILE_DEPTH);
	// ---

	// Draw the tile number (elevation, draw index, etc)
	if (index == (uint64_t)-1)
	{
		char str[10];
		sprintf(str, "%llu", index);
		render_draw_text(x, y - tile.base.elevation * TILE_DEPTH, str, 1, 0x56, 0xB8, 0xFF, 0xFF, ALIGN_CENTER, ALIGN_CENTER, QUALITY_BEST, 0);
	}
	// ---
}

MAP *map_generate_random(uint64_t w, uint64_t h)
{
	MAP *map = NULL;
	uint64_t i, j;

	map = malloc(sizeof(MAP));
	if (map == NULL)
	{
		log_output("map: Insufficient memory\n");
		return NULL;
	}
	memset(map, 0, sizeof(MAP));

	map->map = malloc(sizeof(TILE) * w * h);
	if (map->map == NULL)
	{
		log_output("map: Insufficient memory\n");
		goto map_generate_random_error_1;
	}
	memset(map->map, 0, sizeof(TILE) * w * h);
	map->map_height = h;
	map->map_width = w;

	for (j = 0; j < h; ++j)
	{
		for (i = 0; i < w; ++i)
		{
			uint64_t index = i + j * w;
			map->map[index].type = 0;
			map->map[index].base.elevation = 0;
			if (system_rand() % 2 == 0)
			{
				map->map[index].base.elevation = system_rand() % 2;
			}
			map->map[index].base.corners = 0;
			map->map[index].base.num_units = 0;
			if (system_rand() % 2 == 0)
			{
				if (system_rand() % 2 == 0)
				{
					map->map[index].base.corners |= (system_rand() % 2 == 0) ? (system_rand() % 2 == 0) ? CORNER_L : CORNER_R : (system_rand() % 2 == 0) ? CORNER_U : CORNER_D;
				}
				if (system_rand() % 2 == 0)
				{
					map->map[index].base.corners |= (system_rand() % 2 == 0) ? (system_rand() % 2 == 0) ? CORNER_L : CORNER_R : (system_rand() % 2 == 0) ? CORNER_U : CORNER_D;
				}
			}
		}
	}

	return map;
map_generate_random_error_1:
	free(map);

	return NULL;
}

MAP *map_generate_blank(uint64_t w, uint64_t h)
{
	uint64_t i, j;
	MAP *map;
	if (w == 0 || h == 0)
	{
		log_output("map: Invalid argument(s)\n");
		return NULL;
	}

	map = malloc(sizeof(MAP));
	if (map == NULL)
	{
		log_output("map: Insufficient memory\n");
		return NULL;
	}
	memset(map, 0, sizeof(MAP));

	if ((map->map = malloc(sizeof(TILE) * w * h)) == NULL)
	{
		log_output("game: Insufficient memory\n");
		goto map_generate_blank_error_1;
	}
	memset(map->map, 0, sizeof(TILE) * w * h);
	map->map_width = w;
	map->map_height = h;

	for (j = 0; j < h; ++j)
	{
		for (i = 0; i < w; ++i)
		{
			uint64_t index = i + j * w;
			map->map[index].type = TYPE_GRASS;
			map->map[index].base.elevation = 0;
			map->map[index].base.corners = CORNER_FLAT;
		}
	}

	return map;
map_generate_blank_error_1:
	free(map);

	return NULL;
}

MAP *map_load(uint64_t index)
{
	MAP *map = NULL;
	char str[MAP_NAME_LEN];
	FILE *file;
	uint32_t version;
	uint64_t width = 0, height = 0;
	uint32_t i, j;

	map = malloc(sizeof(MAP));
	if (map == NULL)
	{
		log_output("map: Insufficient memory\n");
		return NULL;
	}
	memset(map, 0, sizeof(MAP));

	sprintf_s(str, sizeof(str), "resources\\%04llu.pmap", index);

	file = fopen(str, "r");
	if (file == NULL)
	{
		log_output("map: Could not open file %s, likely does not exist\n", str);
		goto map_load_error_1;
	}

	if (fscanf(file, "%lu", &version) < 1)
	{
		log_output("map: Could not read verion number. feof: %i ferror: %i\n", feof(file), ferror(file));
		goto map_load_error_2;
	}
	if (version == 0)
	{
		if (fscanf(file, "%llu %llu", &width, &height) < 2)
		{
			log_output("map: Could not read width & height from file. feof: %i ferror: %i\n", feof(file), ferror(file));
			goto map_load_error_2;
		}

		map->map = malloc(sizeof(TILE) * width * height);
		if (map->map == NULL)
		{
			log_output("map: Insufficient memory\n");
			goto map_load_error_2;
		}
		memset(map->map, 0, sizeof(TILE) * width * height);
		map->map_width = width;
		map->map_height = height;
		for (j = 0; j < height; ++j)
		{
			for (i = 0; i < width; ++i)
			{
				if (fscanf(file, "%lu", &(map->map[i + j * width].base.elevation)) < 1)
				{
					log_output("map: Could not read data from file. feof: %i ferror: %i\n", feof(file), ferror(file));
					goto map_load_error_3;
				}
			}
		}
	}
	else if (version == 1)
	{
		if (fscanf(file, "%llu %llu", &width, &height) < 2)
		{
			log_output("map: Could not read width & height from file. feof: %i ferror: %i\n", feof(file), ferror(file));
			goto map_load_error_2;
		}

		map->map = malloc(sizeof(TILE) * width * height);
		if (map->map == NULL)
		{
			log_output("map: Insufficient memory\n");
			goto map_load_error_2;
		}
		memset(map->map, 0, sizeof(TILE) * width * height);
		map->map_width = width;
		map->map_height = height;
		for (j = 0; j < height; ++j)
		{
			for (i = 0; i < width; ++i)
			{
				if (fscanf(file, "%lu%lu%hhx,", &(map->map[i + j * width].base.type), &(map->map[i + j * width].base.elevation), &(map->map[i + j * width].base.corners)) < 3)
				{
					log_output("map: Could not read data from file. feof: %i ferror: %i\n", feof(file), ferror(file));
					goto map_load_error_3;
				}
			}
		}
	}

	fclose(file);
	return map;
map_load_error_3:
	free(map->map);
map_load_error_2:
	fclose(file);
map_load_error_1:
	free(map);

	return NULL;
}

void map_destroy(MAP *map)
{
	if (map != NULL)
	{
		uint64_t i;
		if (map->grid != NULL)
		{
			pathing_destroy_wall_grid(map->grid);
		}
		for (i = 0; i < NUM_UNIT_DEFAULTS; ++i)
		{
			if (map->unit_meshes[i] != NULL)
			{
				pathing_destroy_mesh(map->unit_meshes[i]);
			}
		}
		if (map->map != NULL)
		{
			free(map->map);
		}
		free(map);
	}
}

uint64_t map_get_width(MAP *map)
{
	return map->map_width;
}

uint64_t map_get_height(MAP *map)
{
	return map->map_height;
}

uint32_t map_get_elevation(MAP *map, uint64_t x, uint64_t y)
{
	if (x >= map->map_width || y >= map->map_height)
	{
		return 0;
	}
	return map->map[x + y * map->map_width].base.elevation;
}

uint8_t map_get_corners(MAP *map, uint64_t x, uint64_t y)
{
	if (x >= map->map_width || y >= map->map_height)
	{
		return 0;
	}
	return map->map[x + y * map->map_width].base.corners;
}

TILE map_get_tile(MAP *map, uint64_t x, uint64_t y)
{
	if (x >= map->map_width || y >= map->map_height)
	{
		TILE blank;
		memset(&blank, 0, sizeof(TILE));
		return blank;
	}
	return map->map[x + y * map->map_width];
}

void map_draw(MAP *map, UNIT_LIST *list, int32_t x_off, int32_t y_off)
{
	uint64_t i, j, k, l, m;
	uint64_t max;
	UNIT_LIST *top = NULL;

	if (map->map_height == 0 || map->map_width == 0)
	{
		return;
	}
	if (map->map_height > map->map_width)
	{
		max = map->map_width;
	}
	else
	{
		max = map->map_height;
	}
	/*
	top = list;
	while (top != NULL)
	{
		double unit_x = top->unit.base.x;
		double unit_y = top->unit.base.y;

		int32_t x_coord, y_coord;
		uint64_t x_start = 0;
		uint64_t y_start = 0;
		uint64_t x_end = map->map_width - 1;
		uint64_t y_end = map->map_height - 1;
		uint64_t x_grid = (uint64_t)unit_x;
		uint64_t y_grid = (uint64_t)unit_y;

		if (x_grid > 1)
		{
			x_start = x_grid - 1;
		}
		if (x_grid < map->map_width - 2)
		{
			x_end = x_grid + 1;
		}

		if (y_grid > 1)
		{
			y_start = y_grid - 1;
		}
		if (y_grid < map->map_height - 2)
		{
			y_end = y_grid + 1;
		}

		for (j = y_start; j <= y_end; ++j)
		{
			for (i = x_start; i <= x_end; ++i)
			{
				if (map_unit_is_on_tile(map, unit_x, unit_y, i, j, top->unit.base.size))
				{
					uint64_t index = i + j * map->map_width;
					int32_t x = (int32_t)(i * (TILE_WIDTH / 2) + TILE_WIDTH / 2 + j * (TILE_WIDTH / 2));
					int32_t y = (int32_t)(j * (TILE_HEIGHT / 2) - i * (TILE_HEIGHT / 2));
					//map_draw_tile(map->map[index], x - x_off, y - y_off, map->map[index].base.elevation);
				}
			}
		}

		map_unit_coords_to_drawing_coords(map, unit_x, unit_y, &x_coord, &y_coord);
		x_coord -= x_off;
		y_coord -= y_off;

		render_draw_unit(0, x_coord, y_coord);

		top = top->next;
	}
	*/
	top = list;
	//uint32_t count = 0;
	for (l = 0; l < map->map_height + map->map_width - 1; ++l)
	{
		for (k = 0; k < max && k < l + 1 && k < map->map_height + map->map_width - l - 1; k++)
		{
			if (l < map->map_width)
			{
				i = map->map_width - 1 - l + k;
				j = k;
			}
			else
			{
				i = k;
				j = l - map->map_width + k + 1;
			}
			uint64_t index = i + j * map->map_width;
			//if (map->map[index].base.num_units == 0)
			//{
				int32_t x = (int32_t)(i * (TILE_WIDTH / 2) + TILE_WIDTH / 2 + j * (TILE_WIDTH / 2));
				int32_t y = (int32_t)(j * (TILE_HEIGHT / 2) - i * (TILE_HEIGHT / 2));
				map_draw_tile(map->map[index], x - x_off, y - y_off, map->map[index].base.elevation);
			//}
			for (m = 0; m < map->map[index].base.num_units; ++m)
			{
				if (top == NULL)
				{
					break;
				}
				double unit_x = top->unit.base.x;
				double unit_y = top->unit.base.y;

				int32_t x_coord, y_coord;
				map_unit_coords_to_drawing_coords(map, unit_x, unit_y, &x_coord, &y_coord);
				x_coord -= x_off;
				y_coord -= y_off;

				render_draw_unit(0, x_coord, y_coord);
				
				//char str[10];
				//sprintf(str, "%lu", count++);
				//render_draw_text(x_coord, y_coord, str, 1, 0x56, 0xB8, 0xFF, 0xFF, ALIGN_CENTER, ALIGN_TOP, QUALITY_BEST, 0);
				
				top = top->next;
			}
		}
	}

	/*for (j = 0; j < map->map_height; ++j)
	{
		for (i = 0; i < map->map_width; ++i)
		{
			uint64_t index = i + j * map->map_width;
			if (map->map[index].base.num_units == 0)
			{
				int32_t x = (int32_t)(i * (TILE_WIDTH / 2) + TILE_WIDTH / 2 + j * (TILE_WIDTH / 2));
				int32_t y = (int32_t)(j * (TILE_HEIGHT / 2) - i * (TILE_HEIGHT / 2));
				map_draw_tile(map->map[index], x - x_off, y - y_off, map->map[index].base.elevation);
			}
		}
	}*/
	
	top = list;
	while (top != NULL)
	{
		if (top->unit.base.selected)
		{
			double unit_x = top->unit.base.x;
			double unit_y = top->unit.base.y;

			int32_t x_coord, y_coord;
			map_unit_coords_to_drawing_coords(map, unit_x, unit_y, &x_coord, &y_coord);
			x_coord -= x_off;
			y_coord -= y_off;

			render_draw_unit(1, x_coord, y_coord);
		}
		top = top->next;
	}
}

void map_unit_coords_to_drawing_coords(MAP *map, double unit_x, double unit_y, int32_t *screen_x, int32_t *screen_y)
{
	int32_t elevation = 0;
	if (screen_x == NULL || screen_y == NULL)
	{
		log_output("map: Parameter was NULL: screen_x = %i, screen_y = %i\n", screen_x, screen_y);
		return;
	}
	if ((int32_t)unit_x >= 0 && (int32_t)unit_x < map->map_width && (int32_t)unit_y >= 0 && (int32_t)unit_y < map->map_height)
	{
		elevation = map->map[(uint64_t)unit_x + (uint64_t)unit_y * map->map_width].base.elevation;
	}
	map_unit_coords_to_logical_coords(unit_x, unit_y, screen_x, screen_y);
	*screen_y -= elevation * TILE_DEPTH;
	*screen_y -= UNIT_HEIGHT / 2;
	*screen_y -= (int32_t)round(map_get_unit_z(unit_x, unit_y, map_get_corners(map, (uint64_t)unit_x, (uint64_t)unit_y)) * TILE_DEPTH);
}

void map_unit_coords_to_logical_coords(double unit_x, double unit_y, int32_t *screen_x, int32_t *screen_y)
{
	if (screen_x == NULL || screen_y == NULL)
	{
		log_output("map: Parameter was NULL: screen_x = %i, screen_y = %i\n", screen_x, screen_y);
		return;
	}
	*screen_x = (int32_t)round(unit_x * (TILE_WIDTH / 2) + unit_y * (TILE_WIDTH / 2));
	*screen_y = (int32_t)round(unit_y * (TILE_HEIGHT / 2) - unit_x * (TILE_HEIGHT / 2));
}

void map_logical_coords_to_unit_coords(int32_t screen_x, int32_t screen_y, double *unit_x, double *unit_y)
{
	if (unit_x == NULL || unit_y == NULL)
	{
		log_output("game: Parameter was NULL: unit_x = %i, unit_y = %i\n", unit_x, unit_y);
		return;
	}

	*unit_x = (double)screen_x / TILE_WIDTH - (double)screen_y / TILE_HEIGHT;
	*unit_y = (double)screen_y / TILE_HEIGHT + (double)screen_x / TILE_WIDTH;
}

double map_get_unit_z(double x, double y, uint8_t corners)
{
	x = fmod(x, 1);
	y = fmod(y, 1);
	switch (corners)
	{
	case CORNER_FLAT:
		return (0.0);
	case CORNER_L:
		if ((-x + 1.0) + (-y + 1.0) > 1.0)
		{
			return ((-x + 1.0) + (-y + 1.0) - 1.0);
		}
		return (0.0);
	case CORNER_R:
		if (x + y > 1.0)
		{
			return (x + y - 1.0);
		}
		return (0.0);
	case CORNER_U:
		if (x + (-y + 1.0) > 1.0)
		{
			return (x + (-y + 1.0) - 1.0);
		}
		return (0.0);
	case CORNER_D:
		if ((-x + 1.0) + y > 1.0)
		{
			return ((-x + 1.0) + y - 1.0);
		}
		return (0.0);
	case CORNER_LU:
		return (-y + 1.0);
	case CORNER_UR:
		return (x);
	case CORNER_RD:
		return (y);
	case CORNER_LD:
		return (-x + 1.0);
	case CORNER_LR:
		if ((-x + 1.0) + (-y + 1.0) > 1.0)
		{
			return ((-x + 1.0) + (-y + 1.0) - 1.0);
		}
		return (x + y - 1.0);
	case CORNER_UD:
		if (x + (-y + 1.0) > 1.0)
		{
			return (x + (-y + 1.0) - 1.0);
		}
		return ((-x + 1.0) + y - 1.0);
	case CORNER_LRD:
		if (x + (-y + 1.0) > 1.0)
		{
			return (1.0);
		}
		return ((-x + 1.0) + y - 1.0);
	case CORNER_LUD:
		if (x + y > 1.0)
		{
			return (1.0);
		}
		return ((-x + 1.0) + (-y + 1.0));
	case CORNER_LUR:
		if ((-x + 1.0) + y > 1.0)
		{
			return (1.0);
		}
		return (x + (-y + 1.0) - 1.0);
	case CORNER_URD:
		if ((-x + 1.0) + (-y + 1.0) > 1.0)
		{
			return (1.0);
		}
		return (x + y - 1.0);
	case CORNER_UR2D:
		return (x + y);
	case CORNER_LU2R:
		return (x + (-y + 1.0));
	case CORNER_L2UD:
		return ((-x + 1.0) + (-y + 1.0));
	case CORNER_LRD2:
		return ((-x + 1.0) + y);
	case CORNER_BASE:
	default:
		return (0.0);
	}
}

void _map_update_unit_count(MAP *map, double x, double y, double radius)
{
	uint64_t x_grid = (uint64_t)x;
	uint64_t y_grid = (uint64_t)y;
	uint64_t x_start = 0;
	uint64_t y_start = 0;
	uint64_t x_end = map->map_width - 1;
	uint64_t y_end = map->map_height - 1;
	uint64_t i, j;

	if (x_grid >= map->map_width || y_grid >= map->map_height)
	{
		return;
	}

	if (x_grid > 1)
	{
		x_start = x_grid - 1;
	}
	if (x_grid < map->map_width - 2)
	{
		x_end = x_grid + 1;
	}

	if (y_grid > 1)
	{
		y_start = y_grid - 1;
	}
	if (y_grid < map->map_height - 2)
	{
		y_end = y_grid + 1;
	}

	for (j = y_start; j <= y_end; ++j)
	{
		for (i = x_start; i <= x_end; ++i)
		{
			if (map_unit_is_on_tile(map, x, y, i, j, radius))
			{
				if (map->map[i + j * map->map_width].base.num_units < 0xFFFFFFFFFFFFFFFF)
				{
					map->map[i + j * map->map_width].base.num_units++;
				}
				else
				{
					// throw some sort of error
				}
			}
		}
	}
}

void map_update_units(MAP *map, UNIT_LIST *unit_list)
{
	UNIT_LIST *follower = unit_list;
	uint64_t i, j;

	for (j = 0; j < map->map_height; ++j)
	{
		for (i = 0; i < map->map_width; ++i)
		{
			map->map[i + j * map->map_width].base.num_units = 0;
		}
	}

	while (follower != NULL)
	{
		_map_update_unit_count(map, follower->unit.base.x, follower->unit.base.y, follower->unit.base.size);
		follower = follower->next;
	}
}

uint8_t map_unit_is_on_tile(MAP *map, double unit_x, double unit_y, uint64_t tile_x, uint64_t tile_y, double radius)
{
	uint64_t current_x;
	uint64_t current_y;
	double x1, x2, y1, y2;
	double off = SQRT_RIGHT_ANGLE_TRI * radius;
	double unit_x_off = (unit_x - off);
	double unit_y_off = (unit_y + off);

	current_x = (uint64_t)unit_x_off;
	current_y = (uint64_t)unit_y_off;
	if (current_x >= map->map_width || current_y >= map->map_height)
	{
		return 0;
	}

	if (current_x == tile_x && current_y == tile_y)
	{
		// We are on the tile requested
		return 1;
	}

	if (tile_x == 0 || tile_x >= map->map_width - 1 || tile_y == 0 || tile_y >= map->map_height - 1)
	{
		return 0;
	}

	if (current_x < tile_x - 1 || current_x > tile_x + 1 || current_y < tile_y - 1 || current_y > tile_y + 1)
	{
		// We are outside of the neigboring tiles
		return 0;
	}

	// We are right next to the tile requested
	x1 = (double)tile_x;
	x2 = (double)tile_x + 1;
	y1 = (double)tile_y;
	y2 = (double)tile_y + 1;

	if (unit_x_off < x1 || unit_x_off > x2 || unit_y_off < y1 || unit_y_off > y2)
	{
		// We do not contact the tile requested
		return 0;
	}

	// We are touching the tile requested
	return 1;
}

void map_set_unit_meshes(MAP *map)
{
	uint64_t i;
	for (i = 0; i < NUM_UNIT_DEFAULTS; ++i)
	{
		if (map->unit_meshes[i] != NULL)
		{
			pathing_destroy_mesh(map->unit_meshes[i]);
		}
		if (map->grid == NULL)
		{
			map->grid = pathing_generate_wall_grid(map);
		}
		map->unit_meshes[i] = pathing_create_mesh(map->grid, unit_defaults[i].base.size);
	}
}

void map_remove_unit_meshes(MAP *map)
{
	uint64_t i;
	for (i = 0; i < NUM_UNIT_DEFAULTS; ++i)
	{
		if (map->unit_meshes[i] != NULL)
		{
			pathing_destroy_mesh(map->unit_meshes[i]);
			map->unit_meshes[i] = NULL;
		}
	}
	if (map->grid != NULL)
	{
		pathing_destroy_wall_grid(map->grid);
	}
}

void map_find_path(MAP *map, UNIT *unit, double x, double y)
{
	NODE_MESH *mesh = NULL;
	WALL_GRID *grid;
	uint64_t start, end;

	if (x < unit->base.size)
	{
		x = unit->base.size + NODE_OFFSET_CONTACT;
	}
	else if (x > map_get_width(map) - unit->base.size - NODE_OFFSET_CONTACT)
	{
		x = (double)map_get_width(map) - unit->base.size - NODE_OFFSET_CONTACT;
	}
	if (y < unit->base.size)
	{
		y = unit->base.size + NODE_OFFSET_CONTACT;
	}
	else if (y > map_get_height(map) - unit->base.size - NODE_OFFSET_CONTACT)
	{
		y = (double)map_get_height(map) - unit->base.size - NODE_OFFSET_CONTACT;
	}

	grid = map->grid;
	mesh = map->unit_meshes[unit->type];

	start = pathing_node_mesh_insert(mesh, unit->base.x, unit->base.y, unit->base.size, grid);
	end = pathing_node_mesh_insert(mesh, x, y, unit->base.size, grid);

	//game_draw_nodes(game, mesh);

	pathing_find_path(mesh, unit, x, y);

	//game_draw_unit_path(game, *unit);

	pathing_node_mesh_remove(mesh, start);
	pathing_node_mesh_remove(mesh, end);
}