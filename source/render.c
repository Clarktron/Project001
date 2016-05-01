#include "render.h"

#include "system.h"
#include "log.h"

#include <SDL.h>
#include <SDL_ttf.h>

#define SCALE_X (1)
#define SCALE_Y (1)

#define NUM_FONTS (2)

struct FONT
{
	const char *path;
	TTF_Font *font;
	uint32_t size;
};

struct FONT _font_list[NUM_FONTS] =
{
	{"font\\font.ttf", NULL, 30},
	{"font\\font.ttf", NULL, 10}
};

SDL_Window *_window = NULL;
SDL_Renderer *_renderer = NULL;

#define FPS (60)
uint8_t _locked = 1;
uint32_t _frame_tick = 0;

void render_setup()
{
	uint32_t i;
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		return;
	}
	if (TTF_Init() != 0)
	{
		return;
	}
	_window = SDL_CreateWindow(PROGRAM_NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * SCALE_X, SCREEN_HEIGHT * SCALE_Y, SDL_WINDOW_ALLOW_HIGHDPI);
	_renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);
	for (i = 0; i < NUM_FONTS; ++i)
	{
		_font_list[i].font = TTF_OpenFont(_font_list[i].path, _font_list[i].size);
	}
	SDL_RenderSetScale(_renderer, SCALE_X, SCALE_Y);
	
	if (_locked)
	{
		_frame_tick = SDL_GetTicks();
	}
}

void render_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t r, uint8_t g, uint8_t b)
{
	SDL_SetRenderDrawColor(_renderer, r, g, b, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawLine(_renderer, x1, y1, x2, y2);
}

void render_circle(int32_t x, int32_t y, double radius, uint8_t r, uint8_t g, uint8_t b)
{
	uint64_t segments = (uint64_t)(sqrt(radius) * 2 + 4);
	SDL_SetRenderDrawColor(_renderer, r, g, b, SDL_ALPHA_OPAQUE);

	double step = (2 * M_PI) / segments;

	if (segments < 2)
	{
		SDL_RenderDrawPoint(_renderer, x, y);
	}
	else
	{
		for (uint64_t i = 0; i < segments; i++)
		{
			int32_t x1 = (int32_t)(radius * cos(i * step) + x);
			int32_t y1 = (int32_t)(radius * sin(i * step) + y);
			int32_t x2 = (int32_t)(radius * cos((i + 1) * step) + x);
			int32_t y2 = (int32_t)(radius * sin((i + 1) * step) + y);

			SDL_RenderDrawLine(_renderer, x1, y1, x2, y2);
		}
	}
}

void render_rectangle(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	SDL_Rect rect = {x, y, w, h};
	SDL_SetRenderDrawColor(_renderer, r, g, b, a);
	SDL_RenderFillRect(_renderer, &rect);
}

void render_draw_text(int32_t x, int32_t y, const char *text, uint32_t font_number, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	SDL_Color color = {r, g, b, a};
	TEXTURE texture;
	SDL_Surface *text_surface = TTF_RenderText_Solid(_font_list[font_number].font, text, color);
	if (text_surface == NULL)
	{
		log_output("render: Could not create texture: %s\n", TTF_GetError());
		return;
	}
	SDL_Texture *temp = SDL_CreateTextureFromSurface(_renderer, text_surface);
	if (temp == NULL)
	{
		log_output("render: Could not create texture from surface: %s\n", TTF_GetError());
		SDL_FreeSurface(text_surface);
		return;
	}
	texture.texture = temp;
	texture.w = text_surface->w;
	texture.h = text_surface->h;
	SDL_FreeSurface(text_surface);

	render_draw_texture(texture, x, y);

	render_delete_texture(texture);
}

void render_draw_texture(TEXTURE texture, int32_t x, int32_t y)
{
	SDL_Rect src = {0, 0, texture.w, texture.h};
	SDL_Rect dst = {x, y, texture.w, texture.h};

	SDL_RenderCopy(_renderer, texture.texture, &src, &dst);
}

void render_delete_texture(TEXTURE texture)
{
	SDL_DestroyTexture(texture.texture);
}

void render_begin_frame()
{
	SDL_SetRenderDrawColor(_renderer, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(_renderer);
}

void render_end_frame()
{
	SDL_RenderPresent(_renderer);
	if (_locked)
	{
		uint32_t elapsed_time = SDL_GetTicks() - _frame_tick;
		if (elapsed_time < 1000.0 / FPS)
		{
			SDL_Delay((uint32_t)(1000.0 / FPS) - elapsed_time);
		}
		_frame_tick = SDL_GetTicks();
	}
}

void render_teardown()
{
	SDL_DestroyRenderer(_renderer);
	SDL_DestroyWindow(_window);
	TTF_Quit();
	SDL_Quit();
}
