#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>
#include <SDL.h>

#define ALIGN_LEFT (0x00)
#define ALIGN_RIGHT (0x01)
#define ALIGN_CENTER (0x02)
#define ALIGN_TOP (0x00)
#define ALIGN_BOTTOM (0x01)

#define QUALITY_FAST (0x00)
#define QUALITY_BEST (0x01)

#define OUTLINE (0x00)
#define FILLED (0x01)

#define TILE_WIDTH (40)
#define TILE_HEIGHT (20)
#define TILE_DEPTH (5)

#define UNIT_WIDTH (10)
#define UNIT_HEIGHT (10)

#define SCALE_X (2)
#define SCALE_Y (2)

typedef struct render RENDER_S;

RENDER_S *render_setup();
void render_teardown(RENDER_S *render);
void render_line(RENDER_S *render, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t r, uint8_t g, uint8_t b);
void render_circle(RENDER_S *render, int32_t x, int32_t y, double radius, uint8_t r, uint8_t g, uint8_t b);
void render_rectangle(RENDER_S *render, int32_t x, int32_t y, int32_t w, int32_t h, uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t filled);
void render_draw_text(RENDER_S *render, int32_t x, int32_t y, const char *text, uint32_t font_number, uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t x_alignment, uint8_t y_alignment, uint8_t quality);
void render_draw_slope(RENDER_S *render, uint8_t index, int32_t x, int32_t y);
void render_draw_unit(RENDER_S *render, uint8_t index, int32_t x, int32_t y);
void render_begin_frame(RENDER_S *render);
void render_end_frame(RENDER_S *render);

#endif
