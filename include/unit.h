#ifndef UNIT_H
#define UNIT_H

#include <stdint.h>

#define NUM_UNIT_DEFAULTS (2)

typedef struct unit_path UNIT_PATH;

struct unit_path
{
	UNIT_PATH *next;
	double x;
	double y;
};

typedef enum unit_type
{
	TYPE_TANK,
	TYPE_GUNNER
} UNIT_TYPE;

typedef struct unit_base
{
	UNIT_TYPE type;
	uint8_t selected;
	UNIT_PATH *path;
	double x, y;
	double max_speed;
	double speed;
	double accel;
	double size;
	uint64_t max_health;
	uint64_t health;
	uint64_t attack;
	uint64_t defense;
} UNIT_BASE;

typedef struct unit_tank
{
	UNIT_TYPE type;
	uint8_t selected;
	UNIT_PATH *path;
	double x, y;
	double max_speed;
	double speed;
	double accel;
	double size;
	uint64_t max_health;
	uint64_t health;
	uint64_t attack;
	uint64_t defense;
	uint64_t bullet_speed;
	uint64_t accuracy;
} UNIT_TANK;

typedef struct unit_gunner
{
	UNIT_TYPE type;
	uint8_t selected;
	UNIT_PATH *path;
	double x, y;
	double max_speed;
	double speed;
	double accel;
	double size;
	uint64_t max_health;
	uint64_t health;
	uint64_t attack;
	uint64_t defense;
	uint64_t bullet_speed;
	uint64_t accuracy;
} UNIT_GUNNER;

typedef union unit
{
	UNIT_TYPE type;
	UNIT_BASE base;
	UNIT_TANK tank;
	UNIT_GUNNER gunner;
} UNIT;

typedef struct unit_list UNIT_LIST;

struct unit_list
{
	UNIT_LIST *next;
	UNIT_LIST *prev;
	UNIT unit;
};

const extern UNIT unit_defaults[NUM_UNIT_DEFAULTS];

UNIT_BASE unit_create_base(double x, double y, double max_speed, double speed, double accel, double size, uint64_t max_health, uint64_t health, uint64_t attack, uint64_t defense);
UNIT_GUNNER unit_create_gunner(UNIT_BASE base, uint64_t bullet_speed, uint64_t accuracy);
void unit_insert(UNIT_LIST **unit_list, UNIT_LIST **end, UNIT new_unit);

void unit_msort_unit_list(UNIT_LIST **unit_list, UNIT_LIST **end);

void unit_path_add(UNIT *unit, double x, double y);
void unit_path_add_front(UNIT *unit, double x, double y);
void unit_path_remove(UNIT *unit);
void unit_path_delete(UNIT *unit);
void unit_path_step(UNIT *unit);

#endif
