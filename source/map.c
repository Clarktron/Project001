#include "map.h"
#include "log.h"
#include "render.h"
#include "unit.h"
#include "building.h"
#include "system.h"
#include "pathing.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define MAP_NAME_LEN (260)

struct map
{
	TILE *map;
	int32_t x_offset;
	int32_t y_offset;
	DIM_GRAN map_width;
	DIM_GRAN map_height;
	WALL_GRID *grid;
	NODE_MESH *mesh;
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

MAP *map_generate_random(DIM_GRAN w, DIM_GRAN h)
{
	MAP *map = NULL;
	DIM_GRAN i, j;

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
			if (system_rand() % 10 == 0)
			{
				map->map[index].base.elevation = system_rand() % 10;
			}
			map->map[index].base.corners = 0;
			map->map[index].base.num_units = 0;
			if (system_rand() % 10 == 0)
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

MAP *map_generate_blank(DIM_GRAN w, DIM_GRAN h)
{
	DIM_GRAN i, j;
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
			DIM_GRAN index = i + j * w;
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
	DIM_GRAN width = 0, height = 0;
	DIM_GRAN i, j;

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
		if (fscanf(file, "%i %i", &width, &height) < 2)
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
		if (fscanf(file, "%i %i", &width, &height) < 2)
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
		if (map->grid != NULL)
		{
			pathing_destroy_wall_grid(map->grid);
		}
		if (map->map != NULL)
		{
			free(map->map);
		}
		if (map->mesh != NULL)
		{
			free(map->mesh);
		}
		free(map);
	}
}

DIM_GRAN map_get_width(MAP *map)
{
	return map->map_width;
}

DIM_GRAN map_get_height(MAP *map)
{
	return map->map_height;
}

void map_get_offset(MAP *map, int32_t *x_offset, int32_t *y_offset)
{
	*x_offset = map->x_offset;
	*y_offset = map->y_offset;
}

void map_set_offset(MAP *map, int32_t x_offset, int32_t y_offset)
{
	map->x_offset = x_offset;
	map->y_offset = y_offset;
}

uint32_t map_get_elevation(MAP *map, DIM_GRAN x, DIM_GRAN y)
{
	if (x >= map->map_width || y >= map->map_height)
	{
		return 0;
	}
	return map->map[x + y * map->map_width].base.elevation;
}

uint8_t map_get_corners(MAP *map, DIM_GRAN x, DIM_GRAN y)
{
	if (x < 0 || x < 0 || x >= map->map_width || y >= map->map_height)
	{
		return 0;
	}
	return map->map[x + y * map->map_width].base.corners;
}

TILE map_get_tile(MAP *map, DIM_GRAN x, DIM_GRAN y)
{
	if (x < 0 || y < 0 || x >= map->map_width || y >= map->map_height)
	{
		TILE blank;
		memset(&blank, 0, sizeof(TILE));
		return blank;
	}
	return map->map[x + y * map->map_width];
}

void map_draw(MAP *map, UNIT_LIST *unit_list, BUILDING_LIST *building_list)
{
	DIM_GRAN i, j, k, l, m;
	DIM_GRAN max;
	UNIT_LIST *unit_top = NULL;
	BUILDING_LIST *building_top = NULL;

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

	//uint32_t count = 0;
	unit_top = unit_list;
	building_top = building_list;
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
			UNIT_LIST *temp = unit_top;

			DIM_GRAN index = i + j * map->map_width;
			DIM_GRAN x = i * (TILE_WIDTH / 2) + TILE_WIDTH / 2 + j * (TILE_WIDTH / 2);
			DIM_GRAN y = j * (TILE_HEIGHT / 2) - i * (TILE_HEIGHT / 2);
			if (x - map->x_offset + TILE_WIDTH / 2 >= 0 && y - map->y_offset + TILE_HEIGHT / 2 >= 0 && x - map->x_offset - TILE_WIDTH / 2 < SCREEN_WIDTH && y - map->y_offset - TILE_HEIGHT / 2 - (int32_t)map->map[index].base.elevation * TILE_DEPTH < SCREEN_HEIGHT)
			{
				map_draw_tile(map->map[index], x - map->x_offset, y - map->y_offset, map->map[index].base.num_units);
			}
			for (m = 0; m < map->map[index].base.num_buildings; ++m)
			{
				DIM building_x = world_dim_builder(building_top->building.base.x, 0);
				DIM building_y = world_dim_builder(building_top->building.base.y, 0);

				int32_t x_coord, y_coord;
				map_unit_coords_to_drawing_coords(map, building_x, building_y, &x_coord, &y_coord);

				if (x_coord + BUILDING_WIDTH >= 0 && y_coord + BUILDING_HEIGHT >= 0 && x_coord - BUILDING_WIDTH < SCREEN_WIDTH && y_coord - BUILDING_HEIGHT < SCREEN_HEIGHT)
				{
					render_draw_building(0, x_coord, y_coord);
				}

				building_top = building_top->next;
			}
			temp = unit_top;
			for (m = 0; m < map->map[index].base.num_units; ++m)
			{
				if (temp->unit.base.selected)
				{
					DIM unit_x = temp->unit.base.x;
					DIM unit_y = temp->unit.base.y;

					int32_t x_coord, y_coord;
					map_unit_coords_to_drawing_coords(map, unit_x, unit_y, &x_coord, &y_coord);

					if (x_coord + UNIT_WIDTH >= 0 && y_coord + UNIT_HEIGHT >= 0 && x_coord - UNIT_WIDTH < SCREEN_WIDTH && y_coord - UNIT_HEIGHT < SCREEN_HEIGHT)
					{
						map_draw_circle(map, unit_x, unit_y, temp->unit.base.size);
					}
				}

				temp = temp->next;
			}
			temp = unit_top;
			for (m = 0; m < map->map[index].base.num_units; ++m)
			{
				DIM unit_x = temp->unit.base.x;
				DIM unit_y = temp->unit.base.y;

				int32_t x_coord, y_coord;
				map_unit_coords_to_drawing_coords(map, unit_x, unit_y, &x_coord, &y_coord);

				if (x_coord + UNIT_WIDTH >= 0 && y_coord + UNIT_HEIGHT >= 0 && x_coord - UNIT_WIDTH < SCREEN_WIDTH && y_coord - UNIT_HEIGHT < SCREEN_HEIGHT)
				{
					render_draw_unit(0, x_coord, y_coord);
				}

				//char str[10];
				//sprintf(str, "%lu", count++);
				//render_draw_text(x_coord, y_coord, str, 1, 0x56, 0xB8, 0xFF, 0xFF, ALIGN_CENTER, ALIGN_TOP, QUALITY_BEST, 0);

				temp = temp->next;
			}
			for (m = 0; m < map->map[index].base.num_units; ++m)
			{
				unit_top = unit_top->next;
			}
		}
	}

	// draw selected buildings
	building_top = building_list;
	while (building_top != NULL)
	{
		if (building_top->building.base.selected)
		{
			DIM_GRAN building_x = building_top->building.base.x;
			DIM_GRAN building_y = building_top->building.base.y;

			int32_t x_coord, y_coord;
			map_unit_coords_to_drawing_coords(map, building_x, building_y, &x_coord, &y_coord);
			x_coord -= map->x_offset;
			y_coord -= map->y_offset;

			render_draw_building(1, x_coord, y_coord);
		}
		building_top = building_top->next;
	}
}

void map_unit_coords_to_drawing_coords(MAP *map, DIM unit_x, DIM unit_y, int32_t *screen_x, int32_t *screen_y)
{
	int32_t elevation = 0;
	DIM_GRAN x_gran = world_get_dim_gran(unit_x);
	DIM_GRAN y_gran = world_get_dim_gran(unit_y);

	if (screen_x == NULL || screen_y == NULL)
	{
		log_output("map: Parameter was NULL: screen_x = %i, screen_y = %i\n", screen_x, screen_y);
		return;
	}
	if (x_gran >= 0 && x_gran < map->map_width && y_gran >= 0 && y_gran < map->map_height)
	{
		elevation = map->map[x_gran + y_gran * map->map_width].base.elevation;
	}
	map_unit_coords_to_logical_coords(unit_x, unit_y, screen_x, screen_y);
	*screen_x -= map->x_offset;
	*screen_y -= map->y_offset;
	*screen_y -= elevation * TILE_DEPTH;
	*screen_y -= UNIT_HEIGHT / 2;
	*screen_y -= world_dim_round(map_get_unit_z(unit_x, unit_y, map_get_corners(map, x_gran, y_gran)) * TILE_DEPTH);
}

void map_unit_coords_to_ground_coords(MAP *map, DIM unit_x, DIM unit_y, int32_t *screen_x, int32_t *screen_y)
{
	int32_t elevation = 0;
	DIM_GRAN x_gran = world_get_dim_gran(unit_x);
	DIM_GRAN y_gran = world_get_dim_gran(unit_y);

	if (screen_x == NULL || screen_y == NULL)
	{
		log_output("map: Parameter was NULL: screen_x = %i, screen_y = %i\n", screen_x, screen_y);
		return;
	}
	if (x_gran >= 0 && x_gran < map->map_width && y_gran >= 0 && y_gran < map->map_height)
	{
		elevation = map->map[x_gran + y_gran * map->map_width].base.elevation;
	}
	map_unit_coords_to_logical_coords(unit_x, unit_y, screen_x, screen_y);
	*screen_x -= map->x_offset;
	*screen_y -= map->y_offset;
	*screen_y -= elevation * TILE_DEPTH;
	*screen_y -= world_dim_round(map_get_unit_z(unit_x, unit_y, map_get_corners(map, x_gran, y_gran)) * TILE_DEPTH);
}

void map_unit_coords_to_logical_coords(DIM unit_x, DIM unit_y, int32_t *screen_x, int32_t *screen_y)
{
	if (screen_x == NULL || screen_y == NULL)
	{
		log_output("map: Parameter was NULL: screen_x = %i, screen_y = %i\n", screen_x, screen_y);
		return;
	}
	*screen_x = world_dim_round(unit_x * (TILE_WIDTH / 2) + unit_y * (TILE_WIDTH / 2));
	*screen_y = world_dim_round(unit_y * (TILE_HEIGHT / 2) - unit_x * (TILE_HEIGHT / 2));
}

void map_logical_coords_to_unit_coords(int32_t screen_x, int32_t screen_y, DIM *unit_x, DIM *unit_y)
{
	if (unit_x == NULL || unit_y == NULL)
	{
		log_output("game: Parameter was NULL: unit_x = %i, unit_y = %i\n", unit_x, unit_y);
		return;
	}

	*unit_x = world_dim_builder(screen_x, 0) / TILE_WIDTH - world_dim_builder(screen_y, 0) / TILE_HEIGHT;
	*unit_y = world_dim_builder(screen_y, 0) / TILE_HEIGHT + world_dim_builder(screen_x, 0) / TILE_WIDTH;
}

DIM map_get_unit_z(DIM x, DIM y, uint8_t corners)
{
	DIM_FINE x_fine = world_get_dim_fine(x);
	DIM_FINE y_fine = world_get_dim_fine(y);
	DIM one = world_dim_builder(1, 0);
	switch (corners)
	{
	case CORNER_FLAT:
		return (0);
	case CORNER_L:
		if ((-x_fine + one) + (-y_fine + one) > one)
		{
			return ((-x_fine + one) + (-y_fine + one) - one);
		}
		return (0);
	case CORNER_R:
		if (x_fine + y_fine > one)
		{
			return (x_fine + y_fine - one);
		}
		return (0);
	case CORNER_U:
		if (x_fine + (-y_fine + one) > one)
		{
			return (x_fine + (-y_fine + one) - one);
		}
		return (0);
	case CORNER_D:
		if ((-x_fine + one) + y_fine > one)
		{
			return ((-x_fine + one) + y_fine - one);
		}
		return (0);
	case CORNER_LU:
		return (-y_fine + one);
	case CORNER_UR:
		return (x_fine);
	case CORNER_RD:
		return (y_fine);
	case CORNER_LD:
		return (-x_fine + one);
	case CORNER_LR:
		if ((-x_fine + one) + (-y_fine + one) > one)
		{
			return ((-x_fine + one) + (-y_fine + one) - one);
		}
		return (x_fine + y_fine - one);
	case CORNER_UD:
		if (x_fine + (-y_fine + one) > one)
		{
			return (x_fine + (-y_fine + one) - one);
		}
		return ((-x_fine + one) + y_fine - one);
	case CORNER_LRD:
		if (x_fine + (-y_fine + one) > one)
		{
			return (one);
		}
		return ((-x_fine + one) + y_fine - one);
	case CORNER_LUD:
		if (x_fine + y_fine > one)
		{
			return (one);
		}
		return ((-x_fine + one) + (-y_fine + one));
	case CORNER_LUR:
		if ((-x_fine + one) + y_fine > one)
		{
			return (one);
		}
		return (x_fine + (-y_fine + one) - one);
	case CORNER_URD:
		if ((-x_fine + one) + (-y_fine + one) > one)
		{
			return (one);
		}
		return (x_fine + y_fine - one);
	case CORNER_UR2D:
		return (x_fine + y_fine);
	case CORNER_LU2R:
		return (x_fine + (-y_fine + one));
	case CORNER_L2UD:
		return ((-x_fine + one) + (-y_fine + one));
	case CORNER_LRD2:
		return ((-x_fine + one) + y_fine);
	case CORNER_BASE:
	default:
		return (0);
	}
}

void _map_update_unit_count(MAP *map, DIM x, DIM y, DIM radius)
{
	DIM_GRAN x_grid = world_get_dim_gran(x);
	DIM_GRAN y_grid = world_get_dim_gran(y);
	DIM_GRAN x_start = 0;
	DIM_GRAN y_start = 0;
	DIM_GRAN x_end = map->map_width - 1;
	DIM_GRAN y_end = map->map_height - 1;
	DIM_GRAN i, j;

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
	DIM_GRAN i, j;

	for (j = 0; j < map->map_height; ++j)
	{
		for (i = 0; i < map->map_width; ++i)
		{
			map->map[i + j * map->map_width].base.num_units = 0;
		}
	}

	while (follower != NULL)
	{
		DIM_GRAN x = follower->unit.base.x;
		DIM_GRAN y = follower->unit.base.y;
		_map_update_unit_count(map, x, y, follower->unit.base.size);
		follower = follower->next;
	}
}

void map_update_buildings(MAP *map, BUILDING_LIST *building_list)
{
	BUILDING_LIST *follower = building_list;
	DIM_GRAN i, j;

	for (j = 0; j < map->map_height; ++j)
	{
		for (i = 0; i < map->map_width; ++i)
		{
			map->map[i + j * map->map_width].base.num_buildings = 0;
		}
	}

	while (follower != NULL)
	{
		DIM_GRAN x = follower->building.base.x;
		DIM_GRAN y = follower->building.base.y;
		map->map[x + y * map->map_width].base.num_buildings++;
		follower = follower->next;
	}
}

uint8_t map_unit_is_on_tile(MAP *map, DIM unit_x, DIM unit_y, DIM_GRAN tile_x, DIM_GRAN tile_y, DIM radius)
{
	DIM_GRAN current_x;
	DIM_GRAN current_y;
	DIM x1, x2, y1, y2;
	DIM off = radius;// world_dim_mult(world_sqrt_right_angle_tri, radius);
	DIM unit_x_off = (unit_x - off);
	DIM unit_y_off = (unit_y + off);

	current_x = world_get_dim_gran(unit_x_off);
	current_y = world_get_dim_gran(unit_y_off);
	if (current_x < 0 || current_y < 0 || current_x >= map->map_width || current_y >= map->map_height)
	{
		return 0;
	}

	if (current_x == tile_x && current_y == tile_y)
	{
		// We are on the tile requested
		return 1;
	}
	return 0;

	if (tile_x == 0 || tile_x >= map->map_width - 1 || tile_y == 0 || tile_y >= map->map_height - 1)
	{
		//return 0;
	}

	if (current_x < tile_x - 1 || current_x > tile_x + 1 || current_y < tile_y - 1 || current_y > tile_y + 1)
	{
		// We are outside of the neigboring tiles
		return 0;
	}

	// We are right next to the tile requested
	x1 = world_dim_builder(tile_x, 0);
	x2 = world_dim_builder(tile_x + 1, 0);
	y1 = world_dim_builder(tile_y, 0);
	y2 = world_dim_builder(tile_y + 1, 0);

	if (unit_x_off < x1 || unit_x_off > x2 || unit_y_off < y1 || unit_y_off > y2)
	{
		// We do not contact the tile requested
		return 0;
	}

	// We are touching the tile requested
	return 1;
}

void map_find_path(MAP *map, BUILDING_LIST *building_list, UNIT *unit, DIM x, DIM y)
{
	NODE_MESH *mesh;
	WALL_GRID *grid;
	DIM_GRAN w = map_get_width(map);
	DIM_GRAN h = map_get_height(map);

	if (x < unit->base.size)
	{
		x = unit->base.size + NODE_OFFSET_CONTACT;
	}
	else if (x > world_dim_builder(w, 0) - unit->base.size - NODE_OFFSET_CONTACT)
	{
		x = world_dim_builder(w, 0) - unit->base.size - NODE_OFFSET_CONTACT;
	}
	if (y < unit->base.size)
	{
		y = unit->base.size + NODE_OFFSET_CONTACT;
	}
	else if (y > world_dim_builder(h, 0) - unit->base.size - NODE_OFFSET_CONTACT)
	{
		y = world_dim_builder(h, 0) - unit->base.size - NODE_OFFSET_CONTACT;
	}

	grid = pathing_generate_wall_grid(map, building_list);
	mesh = pathing_create_mesh(grid, w, h, unit->base.size);

	pathing_find_path(mesh, grid, unit, x, y);

	pathing_destroy_wall_grid(grid);
	pathing_destroy_mesh(mesh);
}

void map_draw_circle(MAP *map, DIM x, DIM y, DIM r)
{
	uint64_t segments = 20;

	double step = (2 * M_PI) / segments;

	for (uint64_t i = 0; i < segments; i++)
	{
		DIM x1 = (DIM)(r * cos(i * step) + x);
		DIM y1 = (DIM)(r * sin(i * step) + y);
		DIM x2 = (DIM)(r * cos((i + 1) * step) + x);
		DIM y2 = (DIM)(r * sin((i + 1) * step) + y);

		int32_t xd1, xd2, yd1, yd2;

		map_unit_coords_to_ground_coords(map, x1, y1, &xd1, &yd1);
		map_unit_coords_to_ground_coords(map, x2, y2, &xd2, &yd2);
		
		render_line(xd1, yd1, xd2, yd2, 0xFF, 0xFF, 0xFF, 0);
	}
}
