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

typedef enum unit_action
{
	UNIT_ACTION_NONE,
	UNIT_ACTION_MOVE,
	UNIT_ACTION_ATTACK,
	UNIT_ACTION_ATTACK_MOVE,
	NUM_UNIT_ACTIONS
} UNIT_ACTION;

typedef struct unit_path UNIT_PATH;

struct unit_path
{
	UNIT_PATH *next;
	DIM x;
	DIM y;
};

typedef struct unit_list UNIT_LIST;

typedef struct unit_base
{
	UNIT_TYPE type;
	uint8_t selected;
	UNIT_ACTION action;
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
	DIM range;
	uint32_t attack_delay;
	uint32_t attack_delay_current;
	UNIT_LIST *attack_target;
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

struct unit_list
{
	UNIT_LIST *next;
	UNIT_LIST *prev;
	UNIT unit;
};

const extern UNIT unit_defaults[NUM_UNIT_TYPES];

uint32_t unit_get_num_types();
void unit_get_coords(UNIT *unit, DIM *x, DIM *y);

UNIT unit_create_base(DIM x, DIM y, uint32_t team, SPEED max_speed, SPEED speed, SPEED accel, DIM size, HEALTH max_health, HEALTH health, uint64_t attack, uint64_t defense, DIM range, uint32_t attack_delay);
UNIT unit_create_gunner(UNIT base, uint64_t bullet_speed, uint64_t accuracy);
void unit_insert(UNIT_LIST **unit_list, UNIT_LIST **end, UNIT new_unit);
void unit_remove(UNIT_LIST *unit, UNIT_LIST **start, UNIT_LIST **end);

void unit_msort_unit_list(UNIT_LIST **unit_list, UNIT_LIST **end);

void unit_path_add(UNIT *unit, DIM x, DIM y);
void unit_path_add_front(UNIT *unit, DIM x, DIM y);
void unit_path_remove(UNIT *unit);
void unit_path_delete(UNIT *unit);

void unit_do_action(UNIT *unit);
void unit_attack_target(UNIT *unit, UNIT_LIST *target);

void unit_destroy_dead_units(UNIT_LIST **unit_list, UNIT_LIST **end);

#endif
