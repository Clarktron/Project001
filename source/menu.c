#include "menu.h"
#include "render.h"
#include "input.h"
#include "system.h"

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

#define MENU_MAIN (0)
#define MENU_SINGLE_OPTIONS (1)
#define MENU_MULTI_OPTIONS (2)
#define MENU_GAME (3)
#define MENU_EXIT (0xFFFFFFFF)

uint32_t _menu_number = MENU_MAIN;
uint32_t _menu_previous = MENU_EXIT;
int32_t _click_x = 0;
int32_t _click_y = 0;
uint8_t _clicked = 0;

void menu_mouse_button_event_cb(SDL_Event ev)
{
	if (ev.type == SDL_MOUSEBUTTONDOWN)
	{
		_click_x = ev.button.x;
		_click_y = ev.button.y;
		_clicked = 1;
	}
}

void menu_generic_button(int32_t x, int32_t y, int32_t w, int32_t h, const char *text, uint32_t next_menu)
{
	render_rectangle(x, y, w, h, BUTTON_R, BUTTON_G, BUTTON_B, BUTTON_A);
	render_draw_text(x + w / 2, y + h / 2, text, 0, FONT_R, FONT_G, FONT_B, FONT_A, ALIGN_CENTER, ALIGN_CENTER, QUALITY_BEST);
	if (_clicked && _click_x >= x && _click_x < x + w && _click_y >= y && _click_y < y + h)
	{
		if (_menu_number != next_menu)
		{
			input_unregister_mouse_button_event_cb();
			_menu_number = next_menu;
		}
		_clicked = 0;
	}
}

uint8_t menu_display()
{
	if (_menu_previous != _menu_number)
	{
		input_register_mouse_button_event_cb(*menu_mouse_button_event_cb);
		_menu_previous = _menu_number;
	}
	switch (_menu_number)
	{
		case MENU_MAIN:
			{
				int32_t x_coord = X_OFFSET;
				int32_t y_coord = Y_OFFSET;

				menu_generic_button(x_coord, y_coord, X_WIDTH, Y_HEIGHT, "Single Player", MENU_SINGLE_OPTIONS);
				y_coord += Y_HEIGHT + Y_SEP;
				menu_generic_button(x_coord, y_coord, X_WIDTH, Y_HEIGHT, "Multi Player", MENU_MAIN);
				y_coord += Y_HEIGHT + Y_SEP;
				menu_generic_button(x_coord, y_coord, X_WIDTH, Y_HEIGHT, "Exit", MENU_EXIT);
			}
			break;
		case MENU_SINGLE_OPTIONS:
			{
				int32_t x_coord = X_OFFSET;
				int32_t y_coord = Y_OFFSET;

				menu_generic_button(x_coord, y_coord, X_WIDTH, Y_HEIGHT, "Play", MENU_SINGLE_OPTIONS);
				y_coord += Y_HEIGHT + Y_SEP;

				y_coord += Y_HEIGHT + Y_SEP;
				menu_generic_button(x_coord, y_coord, X_WIDTH, Y_HEIGHT, "Back", MENU_MAIN);
			}
			break;
		case MENU_EXIT:
			return 0;
		default:
			_menu_number = MENU_MAIN;
			break;
	}
	return 1;
}