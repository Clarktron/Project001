#include "render.h"
#include "system.h"
#include "log.h"

#include <stdint.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>

#define NUM_FONTS (2)

#define FPS (60)

typedef struct font
{
	const char *path;
	TTF_Font *font;
	uint32_t size;
} FONT;

struct render
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	uint8_t locked;
	uint32_t frame_tick;
	SDL_Texture *slopes;
	SDL_Texture *units;
	FONT font_list[NUM_FONTS];
};

RENDER_S *render_setup()
{
	RENDER_S *render;
	uint32_t i;
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		log_output("render: Failed to init SDL\n");
		return NULL;
	}
	if (TTF_Init() != 0)
	{
		log_output("render: Failed to init SDL TTF\n");
		SDL_Quit();
		return NULL;
	}
	if (IMG_Init(IMG_INIT_PNG) == 0)
	{
		log_output("render: Failed to init SDL Image\n");
		TTF_Quit();
		SDL_Quit();
		return NULL;
	}
	render = malloc(sizeof(RENDER_S));
	if (render == NULL)
	{
		log_output("render: Insufficient memory\n");
		IMG_Quit();
		TTF_Quit();
		SDL_Quit();
		return NULL;
	}

	render->font_list[0].path = "font\\Consolas.ttf";
	render->font_list[0].size = 20;

	render->font_list[1].path = "font\\Consolas.ttf";
	render->font_list[1].size = 10;

	render->window = SDL_CreateWindow(PROGRAM_NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * SCALE_X, SCREEN_HEIGHT * SCALE_Y, SDL_WINDOW_ALLOW_HIGHDPI);
	if (render->window == NULL)
	{
		log_output("render: Failed to create window\n");
		IMG_Quit();
		TTF_Quit();
		SDL_Quit();
		free(render);
		return NULL;
	}
	render->renderer = SDL_CreateRenderer(render->window, -1, SDL_RENDERER_ACCELERATED);
	if (render->renderer == NULL)
	{
		log_output("render: Failed to create renderer\n");
		SDL_DestroyWindow(render->window);
		IMG_Quit();
		TTF_Quit();
		SDL_Quit();
		free(render);
		return NULL;
	}

	render->slopes = IMG_LoadTexture(render->renderer, "resources\\green_slopes.png");
	if (render->slopes == NULL)
	{
		log_output("render: Failed to load slopes\n");
		SDL_DestroyWindow(render->window);
		SDL_DestroyRenderer(render->renderer);
		IMG_Quit();
		TTF_Quit();
		SDL_Quit();
		free(render);
		return NULL;
	}

	render->units = IMG_LoadTexture(render->renderer, "resources\\units.png");
	if (render->units == NULL)
	{
		log_output("render: Failed to load units\n");
		SDL_DestroyTexture(render->slopes);
		SDL_DestroyWindow(render->window);
		SDL_DestroyRenderer(render->renderer);
		IMG_Quit();
		TTF_Quit();
		SDL_Quit();
		free(render);
		return NULL;
	}

	for (i = 0; i < NUM_FONTS; ++i)
	{
		render->font_list[i].font = TTF_OpenFont(render->font_list[i].path, render->font_list[i].size);
	}
	SDL_RenderSetScale(render->renderer, SCALE_X, SCALE_Y);
	
	render->locked = 1;
	if (render->locked)
	{
		render->frame_tick = SDL_GetTicks();
	}

	SDL_CaptureMouse(1);
	log_output("render: Setup complete\n");
	return render;
}

void render_teardown(RENDER_S *render)
{
	SDL_DestroyTexture(render->slopes);
	SDL_DestroyTexture(render->units);
	SDL_DestroyRenderer(render->renderer);
	SDL_DestroyWindow(render->window);
	IMG_Quit();
	TTF_Quit();
	SDL_Quit();
	free(render);
	log_output("render: Teardown complete\n");
}

void render_line(RENDER_S *render, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t r, uint8_t g, uint8_t b)
{
	SDL_SetRenderDrawColor(render->renderer, r, g, b, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawLine(render->renderer, x1, y1, x2, y2);
}

void render_circle(RENDER_S *render, int32_t x, int32_t y, double radius, uint8_t r, uint8_t g, uint8_t b)
{
	uint64_t segments = (uint64_t)(sqrt(radius) * 2 + 4);
	SDL_SetRenderDrawColor(render->renderer, r, g, b, SDL_ALPHA_OPAQUE);

	double step = (2 * M_PI) / segments;

	if (segments < 2)
	{
		SDL_RenderDrawPoint(render->renderer, x, y);
	}
	else
	{
		for (uint64_t i = 0; i < segments; i++)
		{
			int32_t x1 = (int32_t)(radius * cos(i * step) + x);
			int32_t y1 = (int32_t)(radius * sin(i * step) + y);
			int32_t x2 = (int32_t)(radius * cos((i + 1) * step) + x);
			int32_t y2 = (int32_t)(radius * sin((i + 1) * step) + y);

			SDL_RenderDrawLine(render->renderer, x1, y1, x2, y2);
		}
	}
}

void render_rectangle(RENDER_S *render, int32_t x, int32_t y, int32_t w, int32_t h, uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t filled)
{
	SDL_Rect rect = {x, y, w, h};
	SDL_SetRenderDrawColor(render->renderer, r, g, b, a);
	if (filled == FILLED)
	{
		SDL_RenderFillRect(render->renderer, &rect);
	}
	else
	{
		SDL_RenderDrawRect(render->renderer, &rect);
	}
}

void render_draw_text(RENDER_S *render, int32_t x, int32_t y, const char *text, uint32_t font_number, uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t x_alignment, uint8_t y_alignment, uint8_t quality)
{
	SDL_Color color = {r, g, b, a};
	SDL_Texture *texture;
	int32_t w, h;
	int32_t x_mod;
	int32_t y_mod;
	SDL_Surface *text_surface;
	if (quality == QUALITY_BEST)
	{
		text_surface = TTF_RenderText_Blended(render->font_list[font_number].font, text, color);
	}
	else
	{
		text_surface = TTF_RenderText_Solid(render->font_list[font_number].font, text, color);
	}

	if (text_surface == NULL)
	{
		log_output("render: Could not create texture: %s\n", TTF_GetError());
		return;
	}

	texture = SDL_CreateTextureFromSurface(render->renderer, text_surface);
	if (texture == NULL)
	{
		log_output("render: Could not create texture from surface: %s\n", TTF_GetError());
		SDL_FreeSurface(text_surface);
		return;
	}

	w = text_surface->w;
	h = text_surface->h;
	SDL_FreeSurface(text_surface);

	if (x_alignment == ALIGN_RIGHT)
	{
		x_mod = w;
	}
	else if (x_alignment == ALIGN_CENTER)
	{
		x_mod = w / 2;
	}
	else
	{
		x_mod = 0;
	}

	if (y_alignment == ALIGN_BOTTOM)
	{
		y_mod = h;
	}
	else if (y_alignment == ALIGN_CENTER)
	{
		y_mod = h / 2;
	}
	else
	{
		y_mod = 0;
	}

	SDL_Rect src = {0, 0, w, h};
	SDL_Rect dst = {x - x_mod, y - y_mod, w, h};
	SDL_RenderCopy(render->renderer, texture, &src, &dst);

	SDL_DestroyTexture(texture);
}

void render_draw_slope(RENDER_S *render, uint8_t index, int32_t x, int32_t y)
{
	int32_t w = TILE_WIDTH, h = TILE_HEIGHT + TILE_DEPTH * 2;
	SDL_Rect src = {0, 0, w, h};
	SDL_Rect dst = {x - TILE_WIDTH / 2, y - TILE_HEIGHT / 2 - TILE_DEPTH * 2, w, h};

	src.x = (index % 4) * w;
	src.y = (index / 4) * h;

	SDL_RenderCopy(render->renderer, render->slopes, &src, &dst);
}

void render_draw_unit(RENDER_S *render, uint8_t index, int32_t x, int32_t y)
{
	int32_t w = UNIT_WIDTH, h = UNIT_HEIGHT;
	SDL_Rect src = {0, 0, w, h};
	SDL_Rect dst = {x - UNIT_WIDTH / 2, y - UNIT_HEIGHT / 2, w, h};

	src.x = (index % 4) * w;
	src.y = (index / 4) * h;

	SDL_RenderCopy(render->renderer, render->units, &src, &dst);
}

void render_begin_frame(RENDER_S *render)
{
	SDL_SetRenderDrawColor(render->renderer, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(render->renderer);
}

uint32_t time = 0;

void render_end_frame(RENDER_S *render)
{
	SDL_RenderPresent(render->renderer);
	if (render->locked)
	{
		uint32_t elapsed_time = SDL_GetTicks() - render->frame_tick;
		if (elapsed_time < 1000.0 / FPS)
		{
			SDL_Delay((uint32_t)((1000.0 / FPS) - elapsed_time));
		}
		render->frame_tick = SDL_GetTicks();
	}
}
