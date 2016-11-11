#include "render.h"
#include "system.h"
#include "log.h"

#include <stdint.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>

#define NUM_FONTS (3)

#define FPS (60)

typedef struct font
{
	const char *path;
	TTF_Font *font;
	uint32_t size;
} FONT;

typedef struct render
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	uint8_t locked;
	uint32_t frame_tick;
	SDL_Texture *slopes;
	SDL_Texture *units;
	SDL_Texture *buildings;
	FONT font_list[NUM_FONTS];
} RENDER_S;

RENDER_S *render = NULL;

void render_setup()
{
	uint32_t i;
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		log_output("render: Failed to init SDL\n");
		return;
	}
	if (TTF_Init() != 0)
	{
		log_output("render: Failed to init SDL TTF\n");
		goto render_setup_error_1;
	}
	if (IMG_Init(IMG_INIT_PNG) == 0)
	{
		log_output("render: Failed to init SDL Image\n");
		goto render_setup_error_2;
	}
	if (render != NULL)
	{
		free(render);
	}
	render = malloc(sizeof(RENDER_S));
	if (render == NULL)
	{
		log_output("render: Insufficient memory\n");
		goto render_setup_error_3;
	}

	render->font_list[0].path = "font\\Consolas.ttf";
	render->font_list[0].size = 20;

	render->font_list[1].path = "font\\Consolas.ttf";
	render->font_list[1].size = 10;

	render->font_list[2].path = "font\\Consolas.ttf";
	render->font_list[2].size = 8;

	render->window = SDL_CreateWindow(PROGRAM_NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * SCALE_X, SCREEN_HEIGHT * SCALE_Y, SDL_WINDOW_ALLOW_HIGHDPI);
	if (render->window == NULL)
	{
		log_output("render: Failed to create window\n");
		goto render_setup_error_4;
	}
	render->renderer = SDL_CreateRenderer(render->window, -1, SDL_RENDERER_ACCELERATED);
	if (render->renderer == NULL)
	{
		log_output("render: Failed to create renderer\n");
		goto render_setup_error_5;
	}

	render->slopes = IMG_LoadTexture(render->renderer, "resources\\green_slopes.png");
	if (render->slopes == NULL)
	{
		log_output("render: Failed to load slopes\n");
		goto render_setup_error_6;
	}

	render->units = IMG_LoadTexture(render->renderer, "resources\\units.png");
	if (render->units == NULL)
	{
		log_output("render: Failed to load units\n");
		goto render_setup_error_7;
	}

	render->buildings = IMG_LoadTexture(render->renderer, "resources\\buildings.png");
	if (render->buildings == NULL)
	{
		log_output("render: Failed to load buildings\n");
		goto render_setup_error_8;
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
	return;

render_setup_error_8:
	SDL_DestroyTexture(render->units);
render_setup_error_7:
	SDL_DestroyTexture(render->slopes);
render_setup_error_6:
	SDL_DestroyRenderer(render->renderer);
render_setup_error_5:
	SDL_DestroyWindow(render->window);
render_setup_error_4:
	free(render);
render_setup_error_3:
	IMG_Quit();
render_setup_error_2:
	TTF_Quit();
render_setup_error_1:
	SDL_Quit();
}

void render_teardown()
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

void render_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t r, uint8_t g, uint8_t b, uint8_t instant)
{
	SDL_SetRenderDrawColor(render->renderer, r, g, b, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawLine(render->renderer, x1, y1, x2, y2);
	if (instant)
	{
		SDL_RenderPresent(render->renderer);
	}
}

void render_circle(int32_t x, int32_t y, double radius, uint8_t r, uint8_t g, uint8_t b, uint8_t instant)
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
	
	if (instant)
	{
		SDL_RenderPresent(render->renderer);
	}
}

void render_oval(int32_t x, int32_t y, double width, double height, uint8_t r, uint8_t g, uint8_t b, uint8_t instant)
{
	uint64_t segments = (uint64_t)(sqrt((width + height) / 4.0) * 2 + 4);
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
			int32_t x1 = (int32_t)(width * cos(i * step) + x);
			int32_t y1 = (int32_t)(height * sin(i * step) + y);
			int32_t x2 = (int32_t)(width * cos((i + 1) * step) + x);
			int32_t y2 = (int32_t)(height * sin((i + 1) * step) + y);

			SDL_RenderDrawLine(render->renderer, x1, y1, x2, y2);
		}
	}

	if (instant)
	{
		SDL_RenderPresent(render->renderer);
	}
}

void render_rectangle(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t filled)
{
	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
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

void render_draw_text(int32_t x, int32_t y, const char *text, uint32_t font_number, uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t x_alignment, uint8_t y_alignment, uint8_t quality, uint8_t instant)
{
	SDL_Color color;
	color.r = r;
	color.g = g;
	color.b = b;
	color.a = a;
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

	SDL_Rect src;
	src.x = 0;
	src.y = 0;
	src.w = w;
	src.h = h;
	SDL_Rect dst;
	dst.x = x - x_mod;
	dst.y = y - y_mod;
	dst.w = w;
	dst.h = h;
	SDL_RenderCopy(render->renderer, texture, &src, &dst);

	SDL_DestroyTexture(texture);
	if (instant)
	{
		SDL_RenderPresent(render->renderer);
	}
}

void render_draw_slope(uint8_t index, int32_t x, int32_t y)
{
	int32_t w = TILE_WIDTH, h = TILE_HEIGHT + TILE_DEPTH * 2;
	SDL_Rect src;
	src.x = (index % 4) * w;
	src.y = (index / 4) * h;
	src.w = w;
	src.h = h;
	SDL_Rect dst;
	dst.x = x - TILE_WIDTH / 2;
	dst.y = y - TILE_HEIGHT / 2 - TILE_DEPTH * 2;
	dst.w = w;
	dst.h = h;

	SDL_RenderCopy(render->renderer, render->slopes, &src, &dst);
}

void render_draw_unit(uint8_t index, int32_t x, int32_t y)
{
	int32_t w = UNIT_WIDTH, h = UNIT_HEIGHT;
	SDL_Rect src;
	src.x = 0;
	src.y = 0;
	src.w = w;
	src.h = h;
	SDL_Rect dst;
	dst.x = x - UNIT_WIDTH / 2;
	dst.y = y - UNIT_HEIGHT / 2;
	dst.w = w;
	dst.h = h;

	src.x = (index % 4) * w;
	src.y = (index / 4) * h;

	SDL_RenderCopy(render->renderer, render->units, &src, &dst);
}

void render_draw_building(uint8_t index, int32_t x, int32_t y)
{
	int32_t w = BUILDING_WIDTH, h = BUILDING_HEIGHT;
	SDL_Rect src;
	src.x = 0;
	src.y = 0;
	src.w = w;
	src.h = h;
	SDL_Rect dst;
	dst.x = x;
	dst.y = y - BUILDING_HEIGHT / 2;
	dst.w = w;
	dst.h = h;

	src.x = (index % 4) * w;
	src.y = (index / 4) * h;

	SDL_RenderCopy(render->renderer, render->buildings, &src, &dst);
}

void render_begin_frame()
{
	SDL_SetRenderDrawColor(render->renderer, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(render->renderer);
}

void render_end_frame()
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
