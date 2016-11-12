#include "world.h"

#define FINE_SIZE (100000)

const DIM world_sqrt_right_angle_tri = 70711;

DIM_GRAN world_get_dim_gran(DIM d)
{
	return d / FINE_SIZE;
}

DIM_FINE world_get_dim_fine(DIM d)
{
	return d % FINE_SIZE;
}

DIM world_dim_builder(DIM_GRAN g, DIM_FINE f)
{
	return g * FINE_SIZE + f % FINE_SIZE;
}

DIM world_dim_mult(DIM a, DIM b)
{
	int64_t a_l = a, b_l = b;

	return (DIM)((a_l * b_l) / FINE_SIZE);
}

DIM world_dim_div(DIM a, DIM b)
{
	int64_t a_l = a, b_l = b;

	return (DIM)((a_l * FINE_SIZE) / b_l);
}

DIM world_dim_sqrt(DIM d)
{
	DIM guess = 10 * FINE_SIZE;
	DIM prev_guess = 0;

	if (d == 0)
	{
		return 0;
	}

	while (guess != prev_guess)
	{
		int64_t temp;
		int64_t top = (int64_t)world_dim_mult(guess, guess) - (int64_t)d;
		int64_t bottom = 2 * (int64_t)guess;
		
		temp = guess - world_dim_div((DIM)top, (DIM)bottom);

		prev_guess = guess;
		guess = (DIM)temp;
	}

	return guess - 1;
}

DIM world_dim_ceil(DIM d)
{
	DIM_FINE f = world_get_dim_fine(d);
	DIM_GRAN g = world_get_dim_gran(d);

	if (f)
	{
		return (DIM)((g * FINE_SIZE) + 1);
	}
	return (DIM)(g * FINE_SIZE);
}

DIM_GRAN world_dim_round(DIM d)
{
	DIM_FINE f = world_get_dim_fine(d);
	DIM_GRAN g = world_get_dim_gran(d);

	if (f >= FINE_SIZE / 2)
	{
		return g + 1;
	}
	return g;
}