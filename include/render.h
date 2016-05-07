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

typedef struct TEXTURE
{
	SDL_Texture *texture;
	int32_t w;
	int32_t h;
} TEXTURE;

void render_setup();
void render_teardown();
void render_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t r, uint8_t g, uint8_t b);
void render_circle(int32_t x, int32_t y, double radius, uint8_t r, uint8_t g, uint8_t b);
void render_rectangle(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void render_draw_text(int32_t x, int32_t y, const char *text, uint32_t font_number, uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t x_alignment, uint8_t y_alignment, uint8_t quality);
void render_draw_texture(TEXTURE texture, int32_t x, int32_t y);
void render_delete_texture(TEXTURE texture);
void render_begin_frame();
void render_end_frame();

#endif
