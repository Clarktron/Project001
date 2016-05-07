#include "input.h"
#include "render.h"

#include <stdint.h>
#include <SDL.h>

void(*_keyboard_cb)(SDL_KeyboardEvent) = NULL;
void(*_mouse_motion_cb)(SDL_MouseMotionEvent) = NULL;
void(*_mouse_button_cb)(SDL_MouseButtonEvent) = NULL;
void(*_mouse_wheel_cb)(SDL_MouseWheelEvent) = NULL;
uint8_t _enable_cb = 1;

int32_t input_poll()
{
	SDL_Event ev;
	while (SDL_PollEvent(&ev))
	{
		switch (ev.type)
		{
			case SDL_QUIT:
				return 1;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if (_keyboard_cb != NULL && _enable_cb)
				{
					_keyboard_cb(ev.key);
				}
				break;
			case SDL_MOUSEMOTION:
				if (_mouse_motion_cb != NULL && _enable_cb)
				{
					_mouse_motion_cb(ev.motion);
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				if (_mouse_button_cb != NULL && _enable_cb)
				{
					_mouse_button_cb(ev.button);
				}
				break;
			case SDL_MOUSEWHEEL:
				if (_mouse_wheel_cb != NULL && _enable_cb)
				{
					_mouse_wheel_cb(ev.wheel);
				}
				break;
		}
	}
	return 0;
}

void input_register_keyboard_event_cb(void(*cb)(SDL_KeyboardEvent))
{
	if (cb != NULL)
	{
		_keyboard_cb = cb;
	}
}

void input_register_mouse_motion_event_cb(void(*cb)(SDL_MouseMotionEvent))
{
	if (cb != NULL)
	{
		_mouse_motion_cb = cb;
	}
}

void input_register_mouse_button_event_cb(void(*cb)(SDL_MouseButtonEvent))
{
	if (cb != NULL)
	{
		_mouse_button_cb = cb;
	}
}

void input_register_mouse_wheel_event_cb(void(*cb)(SDL_MouseWheelEvent))
{
	if (cb != NULL)
	{
		_mouse_wheel_cb = cb;
	}
}

void input_unregister_keyboard_event_cb()
{
	_keyboard_cb = NULL;
}

void input_unregister_mouse_motion_event_cb()
{
	_mouse_motion_cb = NULL;
}

void input_unregister_mouse_button_event_cb()
{
	_mouse_button_cb = NULL;
}

void input_unregister_mouse_wheel_event_cb()
{
	_mouse_wheel_cb = NULL;
}

void input_enable_cb()
{
	_enable_cb = 1;
}

void input_disable_cb()
{
	_enable_cb = 0;
}