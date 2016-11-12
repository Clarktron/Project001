#ifndef WORLD_H
#define WORLD_H

#include <stdint.h>

typedef int32_t DIM;
typedef int32_t DIM_GRAN;
typedef int32_t DIM_FINE;

typedef uint32_t SPEED;
typedef uint32_t HEALTH;

DIM_GRAN world_get_dim_gran(DIM d);
DIM_FINE world_get_dim_fine(DIM d);
DIM world_dim_builder(DIM_GRAN g, DIM_FINE f);
DIM world_dim_mult(DIM a, DIM b);
DIM world_dim_div(DIM a, DIM b);
DIM world_dim_sqrt(DIM d);
DIM world_dim_ceil(DIM d);
DIM_GRAN world_dim_round(DIM d);

const extern DIM world_sqrt_right_angle_tri;

#endif
