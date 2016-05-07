#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include <SDL.h>

int32_t input_poll();
void input_register_keyboard_event_cb(void(*cb)(SDL_KeyboardEvent));
void input_register_mouse_motion_event_cb(void(*cb)(SDL_MouseMotionEvent));
void input_register_mouse_button_event_cb(void(*cb)(SDL_MouseButtonEvent));
void input_register_mouse_wheel_event_cb(void(*cb)(SDL_MouseWheelEvent));
void input_unregister_keyboard_event_cb();
void input_unregister_mouse_motion_event_cb();
void input_unregister_mouse_button_event_cb();
void input_unregister_mouse_wheel_event_cb();
void input_enable_cb();
void input_disable_cb();

#endif
