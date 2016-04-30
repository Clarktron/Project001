#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>

void render_setup();
void render_teardown();
void render_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t r, uint8_t g, uint8_t b);
void render_circle(int32_t x, int32_t y, double radius, uint8_t r, uint8_t g, uint8_t b);
void render_begin_frame();
void render_end_frame();

#endif
