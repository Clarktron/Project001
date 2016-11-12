#ifndef UNIT_H
#define UNIT_H

#include <stdint.h>

#include "world.h"

typedef enum unit_type
{
	TYPE_TANK,
	TYPE_GUNNER,
	NUM_UNIT_TYPES
} UNIT_TYPE;

typedef struct unit_path UNIT_PATH;

typedef struct unit_base
{
	UNIT_TYPE type;
	uint8_t selected;
	uint32_t team;
	UNIT_PATH *path;
	DIM x, y;
	SPEED max_speed;
	SPEED speed;
	SPEED accel;
	DIM size;
	HEALTH max_health;
	HEALTH health;
	uint64_t attack;
	uint64_t defense;
} UNIT_BASE;

typedef struct unit_tank
{
	UNIT_BASE base;
	uint64_t bullet_speed;
	uint64_t accuracy;
} UNIT_TANK;

typedef struct unit_gunner
{
	UNIT_BASE base;
	uint64_t bullet_speed;
	uint64_t accuracy;
} UNIT_GUNNER;

typedef union unit UNIT;

union unit
{
	UNIT_TYPE type;
	UNIT_BASE base;
	UNIT_TANK tank;
	UNIT_GUNNER gunner;
};

typedef struct unit_list UNIT_LIST;

struct unit_list
{
	UNIT_LIST *next;
	UNIT_LIST *prev;
	UNIT unit;
};

const extern UNIT unit_defaults[NUM_UNIT_TYPES];

uint32_t unit_get_num_types();
void unit_get_coords(UNIT *unit, DIM *x, DIM *y);

UNIT unit_create_base(DIM x, DIM y, uint32_t team, SPEED max_speed, SPEED speed, SPEED accel, DIM size, HEALTH max_health, HEALTH health, uint64_t attack, uint64_t defense);
UNIT unit_create_gunner(UNIT base, uint64_t bullet_speed, uint64_t accuracy);
void unit_insert(UNIT_LIST **unit_list, UNIT_LIST **end, UNIT new_unit);

void unit_msort_unit_list(UNIT_LIST **unit_list, UNIT_LIST **end);

void unit_path_add(UNIT *unit, DIM x, DIM y);
void unit_path_add_front(UNIT *unit, DIM x, DIM y);
void unit_path_remove(UNIT *unit);
void unit_path_delete(UNIT *unit);
void unit_path_step(UNIT *unit);

#endif
