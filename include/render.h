#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>

#include <SDL.h>

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
void render_load_text(TEXTURE *texture, const char *text, uint32_t font_number, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void render_draw_texture(TEXTURE texture, int32_t x, int32_t y);
void render_begin_frame();
void render_end_frame();

#endif
