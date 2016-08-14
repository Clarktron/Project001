#include "menu.h"
#include "render.h"
#include "input.h"
#include "system.h"
#include "game.h"
#include "log.h"

#include <stdint.h>
#include <string.h>

#define X_WIDTH (SCREEN_WIDTH / 3)
#define Y_HEIGHT (SCREEN_HEIGHT / 8)
#define X_OFFSET ((SCREEN_WIDTH - X_WIDTH) / 2)
#define Y_OFFSET (SCREEN_HEIGHT / 4)
#define Y_SEP (Y_HEIGHT / 2)

#define BUTTON_R (0x7F)
#define BUTTON_G (0x7F)
#define BUTTON_B (0x7F)
#define BUTTON_A (0xFF)

#define BUTTON_DOWN_R (0x5F)
#define BUTTON_DOWN_G (0x5F)
#define BUTTON_DOWN_B (0x5F)

#define FONT_R (0x00)
#define FONT_G (0x00)
#define FONT_B (0x00)
#define FONT_A (0xFF)

struct menu
{
	int32_t click_x;
	int32_t click_y;
	uint8_t clicked;
	uint8_t registered;
};

void _menu_mouse_button_event_cb(SDL_MouseButtonEvent ev, void *ptr);
void _menu_generic_button(MENU_S *menu, INPUT_S *input, RENDER_S *render, STATE *state, STATE next_state, int32_t x, int32_t y, int32_t w, int32_t h, const char *text);

void _menu_mouse_button_event_cb(SDL_MouseButtonEvent ev, void *ptr)
{
	MENU_S *menu = (MENU_S *)ptr;
	menu->click_x = ev.x / SCALE_X;
	menu->click_y = ev.y / SCALE_Y;
	if (ev.type == SDL_MOUSEBUTTONUP)
	{
		menu->clicked = 1;
	}
	else
	{
		menu->clicked = 2;
	}
}

void _menu_generic_button(MENU_S *menu, INPUT_S *input, RENDER_S *render, STATE *state, STATE next_state, int32_t x, int32_t y, int32_t w, int32_t h, const char *text)
{
	if (menu->clicked && menu->click_x >= x && menu->click_x < x + w && menu->click_y >= y && menu->click_y < y + h)
	{
		if (menu->clicked == 1)
		{
			render_rectangle(render, x, y, w, h, BUTTON_R, BUTTON_G, BUTTON_B, BUTTON_A, FILLED);
			if (*state != next_state)
			{
				input_unregister_mouse_button_event_cb(input);
				menu->registered = 0;
				log_output("menu: State change (%s->%s)\n", game_state_to_string(*state), game_state_to_string(next_state));
				*state = next_state;
			}
			menu->clicked = 0;
		}
		else if (menu->clicked == 2)
		{
			render_rectangle(render, x, y, w, h, BUTTON_DOWN_R, BUTTON_DOWN_G, BUTTON_DOWN_B, BUTTON_A, FILLED);
		}
	}
	else
	{
		render_rectangle(render, x, y, w, h, BUTTON_R, BUTTON_G, BUTTON_B, BUTTON_A, FILLED);
	}
	render_draw_text(render, x + w / 2, y + h / 2, text, 0, FONT_R, FONT_G, FONT_B, FONT_A, ALIGN_CENTER, ALIGN_CENTER, QUALITY_BEST);
}

MENU_S *menu_setup()
{
	MENU_S *menu;
	menu = malloc(sizeof(MENU_S));
	if (menu == NULL)
	{
		log_output("menu: Insufficient memory\n");
		return NULL;
	}
	memset(menu, 0, sizeof(MENU_S));
	return menu;
}

void menu_teardown(MENU_S *menu)
{
	free(menu);
}

void menu_display(MENU_S *menu, RENDER_S *render, INPUT_S *input, STATE *state)
{
	if (!menu->registered)
	{
		input_register_mouse_button_event_cb(input, *_menu_mouse_button_event_cb, menu);
		menu->registered = 1;
	}
	switch (*state)
	{
		case STATE_MENU_MAIN:
			{
				int32_t x_coord = X_OFFSET;
				int32_t y_coord = Y_OFFSET;

				_menu_generic_button(menu, input, render, state, STATE_MENU_SINGLE_OPTIONS, x_coord, y_coord, X_WIDTH, Y_HEIGHT, "Single Player");
				y_coord += Y_HEIGHT + Y_SEP;
				_menu_generic_button(menu, input, render, state, STATE_MENU_MULTI_OPTIONS, x_coord, y_coord, X_WIDTH, Y_HEIGHT, "Multi Player");
				y_coord += Y_HEIGHT + Y_SEP;
				_menu_generic_button(menu, input, render, state, STATE_EXIT, x_coord, y_coord, X_WIDTH, Y_HEIGHT, "Exit");
			}
			break;
		case STATE_MENU_SINGLE_OPTIONS:
			{
				int32_t x_coord = X_OFFSET;
				int32_t y_coord = Y_OFFSET;

				_menu_generic_button(menu, input, render, state, STATE_GAME_SINGLE_INIT, x_coord, y_coord, X_WIDTH, Y_HEIGHT, "Play Singleplayer");
				y_coord += Y_HEIGHT + Y_SEP;

				y_coord += Y_HEIGHT + Y_SEP;
				_menu_generic_button(menu, input, render, state, STATE_MENU_MAIN, x_coord, y_coord, X_WIDTH, Y_HEIGHT, "Back");
			}
			break;
		case STATE_MENU_MULTI_OPTIONS:
			{
				int32_t x_coord = X_OFFSET;
				int32_t y_coord = Y_OFFSET;

				_menu_generic_button(menu, input, render, state, STATE_GAME_MULTI_INIT, x_coord, y_coord, X_WIDTH, Y_HEIGHT, "Play Multiplayer");
				y_coord += Y_HEIGHT + Y_SEP;

				y_coord += Y_HEIGHT + Y_SEP;
				_menu_generic_button(menu, input, render, state, STATE_MENU_MAIN, x_coord, y_coord, X_WIDTH, Y_HEIGHT, "Back");
			}
			break;
	}
}