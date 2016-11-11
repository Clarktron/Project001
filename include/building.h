#ifndef BUILDING_H
#define BUILDING_H

#include <stdint.h>

typedef enum building_type
{
	TYPE_HOUSE,
	TYPE_HOTEL,
	NUM_BUILDING_TYPES
} BUILDING_TYPE;

typedef struct building_base
{
	BUILDING_TYPE type;
	uint8_t selected;
	uint32_t index;
	uint32_t x, y;
	uint32_t max_health;
	uint32_t health;
} BUILDING_BASE;

typedef struct buliding_house
{
	BUILDING_BASE base;
} BUILDING_HOUSE;

typedef struct buliding_hotel
{
	BUILDING_BASE base;
} BUILDING_HOTEL;

typedef union building
{
	BUILDING_TYPE type;
	BUILDING_BASE base;
	BUILDING_HOUSE house;
	BUILDING_HOTEL hotel;
} BUILDING;

typedef struct building_list BUILDING_LIST;

struct building_list
{
	BUILDING_LIST *next;
	BUILDING_LIST *prev;
	BUILDING building;
};

const extern BUILDING building_defaults[NUM_BUILDING_TYPES];

BUILDING building_create_base(uint32_t x, uint32_t y, uint32_t index, uint32_t max_health, uint32_t health);
BUILDING building_create_house(BUILDING base);
void building_insert(BUILDING_LIST **building_list, BUILDING_LIST **end, BUILDING new_building);
void building_msort_building_list(BUILDING_LIST **building_list, BUILDING_LIST **end);

#endif