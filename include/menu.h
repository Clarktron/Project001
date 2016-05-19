#ifndef MENU_H
#define MENU_H

#include "state.h"
#include "render.h"
#include "input.h"

#include <stdint.h>

typedef struct menu MENU_S;

MENU_S *menu_setup();
void menu_teardown(MENU_S *menu);
void menu_display(MENU_S *menu, RENDER_S *render, INPUT_S *input, STATE *game);

#endif
