#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include <SDL.h>

typedef struct input INPUT_S;

INPUT_S *input_setup();
void input_teardown(INPUT_S *input);
int32_t input_poll(INPUT_S *input);
void input_register_keyboard_event_cb(INPUT_S *input, void(*cb)(SDL_KeyboardEvent, void *), void *ptr);
void input_register_mouse_motion_event_cb(INPUT_S *input, void(*cb)(SDL_MouseMotionEvent, void *), void *ptr);
void input_register_mouse_button_event_cb(INPUT_S *input, void(*cb)(SDL_MouseButtonEvent, void *), void *ptr);
void input_register_mouse_wheel_event_cb(INPUT_S *input, void(*cb)(SDL_MouseWheelEvent, void *), void *ptr);
void input_unregister_keyboard_event_cb(INPUT_S *input);
void input_unregister_mouse_motion_event_cb(INPUT_S *input);
void input_unregister_mouse_button_event_cb(INPUT_S *input);
void input_unregister_mouse_wheel_event_cb(INPUT_S *input);
void input_enable_cb(INPUT_S *input);
void input_disable_cb(INPUT_S *input);

#endif
