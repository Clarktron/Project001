#include "render.h"

#include "system.h"

#include <SDL.h>
#include <SDL_ttf.h>

#define SCALE_X (1)
#define SCALE_Y (1)

SDL_Window *window;
SDL_Renderer *renderer;
TTF_Font *font;

uint8_t render_on;

void render_setup()
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		return;
	}
	window = SDL_CreateWindow("Petri Dish", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * SCALE_X, SCREEN_HEIGHT * SCALE_Y, SDL_WINDOW_ALLOW_HIGHDPI);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	//font = TTF_OpenFont("font.ttf", 30);
	SDL_RenderSetScale(renderer, SCALE_X, SCALE_Y);

	render_on = 1;
}

void render_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t r, uint8_t g, uint8_t b)
{
	if (render_on)
	{
		SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE);
		SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
	}
}

void render_circle(int32_t x, int32_t y, double radius, uint8_t r, uint8_t g, uint8_t b)
{
	if (render_on)
	{
		uint64_t segments = (uint64_t)(sqrt(radius) * 2 + 4);
		SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE);

		double step = (2 * M_PI) / segments;

		if (segments < 2)
		{
			SDL_RenderDrawPoint(renderer, x, y);
		}
		else
		{
			for (uint64_t i = 0; i < segments; i++)
			{
				int32_t x1 = (int32_t)(radius * cos(i * step) + x);
				int32_t y1 = (int32_t)(radius * sin(i * step) + y);
				int32_t x2 = (int32_t)(radius * cos((i + 1) * step) + x);
				int32_t y2 = (int32_t)(radius * sin((i + 1) * step) + y);

				SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
			}
		}
	}
}

void render_text(const char *text)
{

}

void render_begin_frame()
{
	if (render_on)
	{
		SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);
	}
}

void render_end_frame()
{
	if (render_on)
	{
		SDL_RenderPresent(renderer);
	}
}

void render_teardown()
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
