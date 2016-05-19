#include "input.h"
#include "render.h"
#include "log.h"

#include <string.h>
#include <stdint.h>
#include <SDL.h>

struct input
{
	void(*keyboard_cb)(SDL_KeyboardEvent, void *);
	void(*mouse_motion_cb)(SDL_MouseMotionEvent, void *);
	void(*mouse_button_cb)(SDL_MouseButtonEvent, void *);
	void(*mouse_wheel_cb)(SDL_MouseWheelEvent, void *);

	void *keyboard_ptr;
	void *mouse_motion_ptr;
	void *mouse_button_ptr;
	void *mouse_wheel_ptr;

	uint8_t enable_cb;
};

INPUT_S *input_setup()
{
	INPUT_S *input;
	input = malloc(sizeof(INPUT_S));
	if (input == NULL)
	{
		log_output("input: Insufficient memory\n");
		return NULL;
	}

	memset(input, 0, sizeof(INPUT_S));
	input->enable_cb = 1;

	return input;
}

void input_teardown(INPUT_S *input)
{
	free(input);
}

int32_t input_poll(INPUT_S *input)
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
				if (input->keyboard_cb != NULL && input->enable_cb)
				{
					input->keyboard_cb(ev.key, input->keyboard_ptr);
				}
				break;
			case SDL_MOUSEMOTION:
				if (input->mouse_motion_cb != NULL && input->enable_cb)
				{
					input->mouse_motion_cb(ev.motion, input->mouse_motion_ptr);
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				if (input->mouse_button_cb != NULL && input->enable_cb)
				{
					input->mouse_button_cb(ev.button, input->mouse_button_ptr);
				}
				break;
			case SDL_MOUSEWHEEL:
				if (input->mouse_wheel_cb != NULL && input->enable_cb)
				{
					input->mouse_wheel_cb(ev.wheel, input->mouse_wheel_ptr);
				}
				break;
		}
	}
	return 0;
}

void input_register_keyboard_event_cb(INPUT_S *input, void(*cb)(SDL_KeyboardEvent, void *), void *ptr)
{
	if (cb != NULL)
	{
		input->keyboard_cb = cb;
		input->keyboard_ptr = ptr;
		log_output("input: Registered keyboard event callback: 0x%8X\n", cb);
	}
}

void input_register_mouse_motion_event_cb(INPUT_S *input, void(*cb)(SDL_MouseMotionEvent, void *), void *ptr)
{
	if (cb != NULL)
	{
		input->mouse_motion_cb = cb;
		input->mouse_motion_ptr = ptr;
		log_output("input: Registered mouse motion event callback: 0x%8X\n", cb);
	}
}

void input_register_mouse_button_event_cb(INPUT_S *input, void(*cb)(SDL_MouseButtonEvent, void *), void *ptr)
{
	if (cb != NULL)
	{
		input->mouse_button_cb = cb;
		input->mouse_button_ptr = ptr;
		log_output("input: Registered mouse button event callback: 0x%8X\n", cb);
	}
}

void input_register_mouse_wheel_event_cb(INPUT_S *input, void(*cb)(SDL_MouseWheelEvent, void *), void *ptr)
{
	if (cb != NULL)
	{
		input->mouse_wheel_cb = cb;
		input->mouse_wheel_ptr = ptr;
		log_output("input: Registered mouse wheel event callback: 0x%8X\n", cb);
	}
}

void input_unregister_keyboard_event_cb(INPUT_S *input)
{
	log_output("input: Unregistered keyboard event callback: 0x%8X\n", input->keyboard_cb);
	input->keyboard_cb = NULL;
	input->keyboard_ptr = NULL;
}

void input_unregister_mouse_motion_event_cb(INPUT_S *input)
{
	log_output("input: Unregistered mouse motion event callback: 0x%8X\n", input->mouse_motion_cb);
	input->mouse_motion_cb = NULL;
	input->mouse_motion_ptr = NULL;
}

void input_unregister_mouse_button_event_cb(INPUT_S *input)
{
	log_output("input: Unregistered mouse button event callback: 0x%8X\n", input->mouse_button_cb);
	input->mouse_button_cb = NULL;
	input->mouse_button_ptr = NULL;
}

void input_unregister_mouse_wheel_event_cb(INPUT_S *input)
{
	log_output("input: Unregistered mouse wheel event callback: 0x%8X\n", input->mouse_wheel_cb);
	input->mouse_wheel_cb = NULL;
	input->mouse_wheel_ptr = NULL;
}

void input_enable_cb(INPUT_S *input)
{
	input->enable_cb = 1;
}

void input_disable_cb(INPUT_S *input)
{
	input->enable_cb = 0;
}