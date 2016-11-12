#include "building.h"
#include "log.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const BUILDING building_defaults[NUM_BUILDING_TYPES] = 
{
	{
		.house = {{TYPE_HOUSE, 0, 0, 0, 0, 1000, 1000}}
	},
	{
		.hotel = {{TYPE_HOTEL, 0, 0, 0, 0, 1000, 1000}}
	}
};

BUILDING_LIST *_building_msort_building_list_compare(BUILDING_LIST *first, BUILDING_LIST *second);
BUILDING_LIST *_building_msort_building_list_merge(BUILDING_LIST *first, BUILDING_LIST *second);

BUILDING building_create_base(DIM_GRAN x, DIM_GRAN y, uint32_t index, HEALTH max_health, HEALTH health)
{
	BUILDING new_building;

	new_building.base.selected = 0;
	new_building.base.index = index;
	new_building.base.x = x;
	new_building.base.y = y;
	new_building.base.max_health = max_health;
	new_building.base.health = health;

	return new_building;
}

BUILDING building_create_house(BUILDING base)
{
	return base;
}

void building_insert(BUILDING_LIST **building_list, BUILDING_LIST **end, BUILDING new_building)
{
	BUILDING_LIST *follower = *building_list;

	if (follower == NULL)
	{
		*building_list = malloc(sizeof(BUILDING_LIST));
		if (*building_list == NULL)
		{
			log_output("building: Out of memory\n");
			return;
		}
		(*building_list)->next = NULL;
		(*building_list)->prev = NULL;
		(*building_list)->building = new_building;
		*end = *building_list;
	}
	else
	{
		if (*end != NULL)
		{
			follower = *end;
		}
		else
		{
			log_output("building: Failed to use cached unit list ending, parsing through insetad\n");
			while (follower->next != NULL)
			{
				follower = follower->next;
			}
		}
		follower->next = malloc(sizeof(BUILDING_LIST));
		if (follower->next == NULL)
		{
			log_output("building: Insufficient memory\n");
			return;
		}
		follower->next->next = NULL;
		follower->next->prev = follower;
		follower->next->building = new_building;
		*end = follower->next;
	}
}

void building_msort_building_list(BUILDING_LIST **building_list, BUILDING_LIST **end)
{
	BUILDING_LIST *new_head = *building_list, *temp = NULL;
	BUILDING_LIST *list[32];
	memset(list, 0, sizeof(BUILDING_LIST *) * 32);
	BUILDING_LIST *next;
	uint32_t i;
	
	while (new_head != NULL)
	{
		next = new_head->next;
		new_head->next = NULL;
		for (i = 0; (i < 32) && (list[i] != NULL); ++i)
		{
			new_head = _building_msort_building_list_merge(list[i], new_head);
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
		new_head = _building_msort_building_list_merge(list[i], new_head);
	}

	*building_list = new_head;
	temp = new_head;

	while (new_head != NULL)
	{
		temp = new_head;
		new_head = new_head->next;
	}

	*end = temp;
}

BUILDING_LIST *_building_msort_building_list_merge(BUILDING_LIST *first, BUILDING_LIST *second)
{
	BUILDING_LIST *result = NULL;
	BUILDING_LIST *result_end = NULL;
	BUILDING_LIST *first_follow = first, *second_follow = second;

	while (first_follow != NULL && second_follow != NULL)
	{
		BUILDING_LIST *temp = _building_msort_building_list_compare(first_follow, second_follow);
		if (temp == NULL)
		{
			break;
		}

		if (temp == first_follow)
		{
			BUILDING_LIST *next = first_follow->next;
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
			BUILDING_LIST *next = second_follow->next;
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
		BUILDING_LIST *next = first_follow->next;
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
		BUILDING_LIST *next = second_follow->next;
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

BUILDING_LIST *_building_msort_building_list_compare(BUILDING_LIST *first, BUILDING_LIST *second)
{
	int64_t ax, ay, bx, by;

	if (first == NULL)
	{
		return second;
	}
	if (second == NULL)
	{
		return first;
	}

	//sorting criteria
	ax = first->building.base.x;
	ay = first->building.base.y;

	bx = second->building.base.x;
	by = second->building.base.y;

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

	// default to this when all else fails
	return first;
}