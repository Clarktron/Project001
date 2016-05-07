#include "menu.h"
#include "render.h"
#include "input.h"
#include "system.h"
#include "game.h"
#include "log.h"

#include <stdint.h>

#define X_WIDTH (SCREEN_WIDTH / 3)
#define Y_HEIGHT (SCREEN_HEIGHT / 8)
#define X_OFFSET ((SCREEN_WIDTH - X_WIDTH) / 2)
#define Y_OFFSET (SCREEN_HEIGHT / 4)
#define Y_SEP (Y_HEIGHT / 2)

#define BUTTON_R (0x7F)
#define BUTTON_G (0x7F)
#define BUTTON_B (0x7F)
#define BUTTON_A (0xFF)

#define FONT_R (0x00)
#define FONT_G (0x00)
#define FONT_B (0x00)
#define FONT_A (0xFF)

GAME_STATE _prev_state = STATE_EXIT;
int32_t _click_x = 0;
int32_t _click_y = 0;
uint8_t _clicked = 0;

void _menu_mouse_button_event_cb(SDL_MouseButtonEvent ev);
void _menu_generic_button(GAME_STATE *state, GAME_STATE next_state, int32_t x, int32_t y, int32_t w, int32_t h, const char *text);

void _menu_mouse_button_event_cb(SDL_MouseButtonEvent ev)
{
	if (ev.type == SDL_MOUSEBUTTONDOWN)
	{
		_click_x = ev.x;
		_click_y = ev.y;
		_clicked = 1;
	}
}

void _menu_generic_button(GAME_STATE *state, GAME_STATE next_state, int32_t x, int32_t y, int32_t w, int32_t h, const char *text)
{
	render_rectangle(x, y, w, h, BUTTON_R, BUTTON_G, BUTTON_B, BUTTON_A);
	render_draw_text(x + w / 2, y + h / 2, text, 0, FONT_R, FONT_G, FONT_B, FONT_A, ALIGN_CENTER, ALIGN_CENTER, QUALITY_BEST);
	if (_clicked && _click_x >= x && _click_x < x + w && _click_y >= y && _click_y < y + h)
	{
		if (*state != next_state)
		{
			input_unregister_mouse_button_event_cb();
			log_output("menu: State change (%s->%s)\n", game_state_to_string(*state), game_state_to_string(next_state));
			*state = next_state;
		}
		_clicked = 0;
	}
}

void menu_display(GAME_STATE *state)
{
	if (_prev_state != *state)
	{
		input_register_mouse_button_event_cb(*_menu_mouse_button_event_cb);
		_prev_state = *state;
	}
	switch (*state)
	{
		case STATE_MENU_MAIN:
			{
				int32_t x_coord = X_OFFSET;
				int32_t y_coord = Y_OFFSET;

				_menu_generic_button(state, STATE_MENU_SINGLE_OPTIONS, x_coord, y_coord, X_WIDTH, Y_HEIGHT, "Single Player");
				y_coord += Y_HEIGHT + Y_SEP;
				_menu_generic_button(state, STATE_MENU_MULTI_OPTIONS, x_coord, y_coord, X_WIDTH, Y_HEIGHT, "Multi Player");
				y_coord += Y_HEIGHT + Y_SEP;
				_menu_generic_button(state, STATE_EXIT, x_coord, y_coord, X_WIDTH, Y_HEIGHT, "Exit");
			}
			break;
		case STATE_MENU_MULTI_OPTIONS:
		case STATE_MENU_SINGLE_OPTIONS:
			{
				int32_t x_coord = X_OFFSET;
				int32_t y_coord = Y_OFFSET;

				_menu_generic_button(state, STATE_GAME_SINGLE, x_coord, y_coord, X_WIDTH, Y_HEIGHT, "Play");
				y_coord += Y_HEIGHT + Y_SEP;

				y_coord += Y_HEIGHT + Y_SEP;
				_menu_generic_button(state, STATE_MENU_MAIN, x_coord, y_coord, X_WIDTH, Y_HEIGHT, "Back");
			}
			break;
	}
}