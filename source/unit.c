#include "unit.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SQRT_RIGHT_ANGLE_TRI (0.70710678)

const UNIT unit_defaults[NUM_UNIT_DEFAULTS] =
{
	{
		.tank = { TYPE_GUNNER, 0, NULL, 0.0, 0.0, 0.015, 0.0, 0.1, 0.1, 100, 100, 2, 2, 10, 75}
	},
	{
		.gunner = {TYPE_GUNNER, 0, NULL, 0.0, 0.0, 0.015, 0.0, 0.1, 0.1, 100, 100, 2, 2, 10, 75}
	}
};

UNIT_LIST *_unit_msort_unit_list_compare(UNIT_LIST *first, UNIT_LIST *second);
UNIT_LIST *_unit_msort_unit_list_merge(UNIT_LIST *first, UNIT_LIST *second);

UNIT_BASE unit_create_base(double x, double y, double max_speed, double speed, double accel, double size, uint64_t max_health, uint64_t health, uint64_t attack, uint64_t defense)
{
	UNIT_BASE new_unit;

	new_unit.selected = 0;
	new_unit.path = NULL;
	new_unit.x = x;
	new_unit.y = y;
	new_unit.max_speed = max_speed;
	new_unit.speed = speed;
	new_unit.accel = accel;
	new_unit.size = size;
	new_unit.max_health = max_health;
	new_unit.health = health;
	new_unit.attack = attack;
	new_unit.defense = defense;

	return new_unit;
}

UNIT_GUNNER unit_create_gunner(UNIT_BASE base, uint64_t bullet_speed, uint64_t accuracy)
{
	UNIT_GUNNER new_unit;

	new_unit.selected = base.selected;
	new_unit.path = base.path;
	new_unit.x = base.x;
	new_unit.y = base.y;
	new_unit.max_speed = base.max_speed;
	new_unit.speed = base.speed;
	new_unit.accel = base.accel;
	new_unit.size = base.size;
	new_unit.max_health = base.max_health;
	new_unit.health = base.health;
	new_unit.attack = base.attack;
	new_unit.defense = base.defense;

	new_unit.type = TYPE_GUNNER;
	new_unit.bullet_speed = bullet_speed;
	new_unit.accuracy = accuracy;

	return new_unit;
}

void unit_insert(UNIT_LIST **unit_list, UNIT_LIST **end, UNIT new_unit)
{
	UNIT_LIST *follower = *unit_list;

	if (follower == NULL)
	{
		*unit_list = malloc(sizeof(UNIT_LIST));
		if (*unit_list == NULL)
		{
			log_output("unit: Out of memory\n");
			return;
		}
		(*unit_list)->next = NULL;
		(*unit_list)->prev = NULL;
		(*unit_list)->unit = new_unit;
		*end = *unit_list;
	}
	else
	{
		if (*end != NULL)
		{
			follower = *end;
		}
		else
		{
			log_output("unit: Failed to use cached unit list ending, parsing through instead");
			while (follower->next != NULL)
			{
				follower = follower->next;
			}
		}
		follower->next = malloc(sizeof(UNIT_LIST));
		if (follower->next == NULL)
		{
			log_output("unit: Insufficient memory\n");
			return;
		}
		follower->next->next = NULL;
		follower->next->prev = follower;
		follower->next->unit = new_unit;
		*end = follower->next;
	}
}

// merge sort the list of units, so that they appear in drawing order
void unit_msort_unit_list(UNIT_LIST **unit_list, UNIT_LIST **end)
{
	if (unit_list == NULL || *unit_list == NULL)
	{
		return;
	}

	UNIT_LIST *new_head = *unit_list, *temp = NULL;
	UNIT_LIST *list[32];
	memset(list, 0, sizeof(UNIT_LIST *) * 32);
	UNIT_LIST *next;
	uint32_t i;

	while (new_head != NULL)
	{
		next = new_head->next;
		new_head->next = NULL;
		for (i = 0; (i < 32) && (list[i] != NULL); ++i)
		{
			new_head = _unit_msort_unit_list_merge(list[i], new_head);
			list[i] = NULL;
		}
		if (i == 32)
		{
			--i;
		}
		list[i] = new_head;
		new_head = next;
	}

	new_head = NULL;
	for (i = 0; i < 32; ++i)
	{
		new_head = _unit_msort_unit_list_merge(list[i], new_head);
	}

	*unit_list = new_head;
	temp = new_head;

	while (new_head != NULL)
	{
		temp = new_head;
		new_head = new_head->next;
	}

	*end = temp;
}

UNIT_LIST *_unit_msort_unit_list_merge(UNIT_LIST *first, UNIT_LIST *second)
{
	UNIT_LIST *result = NULL;
	UNIT_LIST *result_end = NULL;
	UNIT_LIST *first_follow = first, *second_follow = second;

	while (first_follow != NULL && second_follow != NULL)
	{
		UNIT_LIST *temp = _unit_msort_unit_list_compare(first_follow, second_follow);
		if (temp == NULL)
		{
			break;
		}

		if (temp == first_follow)
		{
			UNIT_LIST *next = first_follow->next;
			if (result == NULL)
			{
				result = first_follow;
				first_follow->prev = NULL;
				first_follow->next = NULL;
				result_end = first_follow;
			}
			else
			{
				result_end->next = first_follow;
				first_follow->prev = result_end;
				first_follow->next = NULL;

				result_end = first_follow;
			}

			first_follow = next;
		}
		else
		{
			UNIT_LIST *next = second_follow->next;
			if (result == NULL)
			{
				result = second_follow;
				second_follow->prev = NULL;
				second_follow->next = NULL;
				result_end = second_follow;
			}
			else
			{
				result_end->next = second_follow;
				second_follow->prev = result_end;
				second_follow->next = NULL;

				result_end = second_follow;
			}

			second_follow = next;
		}
	}

	// either first or second may have content left, empty them
	while (first_follow != NULL)
	{
		UNIT_LIST *next = first_follow->next;
		if (result == NULL)
		{
			result = first_follow;
			first_follow->prev = NULL;
			first_follow->next = NULL;
			result_end = first_follow;
		}
		else
		{
			result_end->next = first_follow;
			first_follow->prev = result_end;
			first_follow->next = NULL;

			result_end = first_follow;
		}

		first_follow = next;
	}

	while (second_follow != NULL)
	{
		UNIT_LIST *next = second_follow->next;
		if (result == NULL)
		{
			result = second_follow;
			second_follow->prev = NULL;
			second_follow->next = NULL;
			result_end = second_follow;
		}
		else
		{
			result_end->next = second_follow;
			second_follow->prev = result_end;
			second_follow->next = NULL;

			result_end = second_follow;
		}

		second_follow = next;
	}

	return result;
}

UNIT_LIST *_unit_msort_unit_list_compare(UNIT_LIST *first, UNIT_LIST *second)
{
	int32_t ax, ay, bx, by;
	double afx, afy, bfx, bfy;
	double aoff, boff;

	if (first == NULL)
	{
		return second;
	}
	if (second == NULL)
	{
		return first;
	}

	aoff = first->unit.base.size * SQRT_RIGHT_ANGLE_TRI;
	boff = second->unit.base.size * SQRT_RIGHT_ANGLE_TRI;

	//sorting criteria
	ax = (int32_t)(first->unit.base.x - aoff);
	ay = (int32_t)(first->unit.base.y + aoff);

	bx = (int32_t)(second->unit.base.x - boff);
	by = (int32_t)(second->unit.base.y + boff);

	// first criteria: which "tile y" is less (render order / topwards)
	if (-ax + ay < -bx + by)
	{
		return first;
	}
	if (-ax + ay > -bx + by)
	{
		return second;
	}

	// second criteria: which "tile x" is less (leftwards)
	if (ax < bx)
	{
		return first;
	}
	if (ax > bx)
	{
		return second;
	}

	// third criteria: which "screen y" is less (topwards)
	afx = first->unit.base.x;
	afy = first->unit.base.y;

	bfx = second->unit.base.x;
	bfy = second->unit.base.y;

	if (-afx + afy < -bfx + bfy)
	{
		return first;
	}
	if (-afx + afy > -bfx + bfy)
	{
		return second;
	}

	// default to this when all else fails
	return first;
}

void unit_path_add(UNIT *unit, double x, double y)
{
	if (unit->base.path == NULL)
	{
		unit->base.path = (UNIT_PATH *)malloc(sizeof(UNIT_PATH));
		if (unit->base.path == NULL)
		{
			log_output("unit: Unable to allocate memory\n");
			return;
		}
		unit->base.path->next = NULL;
		unit->base.path->x = x;
		unit->base.path->y = y;
	}
	else
	{
		UNIT_PATH *seeker = unit->base.path;
		while (seeker->next != NULL)
		{
			seeker = seeker->next;
		}
		seeker->next = (UNIT_PATH *)malloc(sizeof(UNIT_PATH));
		if (seeker->next == NULL)
		{
			log_output("unit: Unable to allocate memory\n");
			return;
		}
		seeker->next = NULL;
		seeker->x = x;
		seeker->y = y;
	}
}

void unit_path_add_front(UNIT *unit, double x, double y)
{
	if (unit->base.path == NULL)
	{
		unit->base.path = (UNIT_PATH *)malloc(sizeof(UNIT_PATH));
		if (unit->base.path == NULL)
		{
			log_output("unit: Unable to allocate memory\n");
			return;
		}
		unit->base.path->next = NULL;
		unit->base.path->x = x;
		unit->base.path->y = y;
	}
	else
	{
		UNIT_PATH *new_path = NULL;

		new_path = (UNIT_PATH *)malloc(sizeof(UNIT_PATH));
		if (new_path == NULL)
		{
			log_output("unit: Unable to allocate memory\n");
			return;
		}

		new_path->next = unit->base.path;
		new_path->x = x;
		new_path->y = y;
		unit->base.path = new_path;
	}
}

void unit_path_remove(UNIT *unit)
{
	if (unit->base.path != NULL)
	{
		UNIT_PATH *next = unit->base.path->next;
		free(unit->base.path);
		unit->base.path = next;
	}
	else
	{
		log_output("unit: Unable to remove path, already empty\n");
	}
}

void unit_path_delete(UNIT *unit)
{
	UNIT_PATH *seeker = unit->base.path;
	while (seeker != NULL)
	{
		UNIT_PATH *trailer = seeker;
		seeker = seeker->next;
		free(trailer);
	}
	unit->base.path = NULL;
}

void unit_path_step(UNIT *unit)
{
	if (unit->base.path != NULL)
	{
		double x_sep = unit->base.path->x - unit->base.x;
		double y_sep = unit->base.path->y - unit->base.y;
		double len = sqrt((x_sep * x_sep) + (y_sep * y_sep));
		double steps = ceil(len / unit->base.max_speed);
		if (steps > 1)
		{
			unit->base.x += x_sep / steps;
			unit->base.y += y_sep / steps;
		}
		else
		{
			unit->base.x = unit->base.path->x;
			unit->base.y = unit->base.path->y;
			unit_path_remove(unit);
		}
	}
}