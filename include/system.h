#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include <SDL.h>

#define SCREEN_WIDTH (640)
#define SCREEN_HEIGHT (480)

void system_setup();
void system_teardown();
int32_t system_rand();

#endif
