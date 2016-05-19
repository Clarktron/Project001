#include "render.h"
#include "system.h"
#include "log.h"

#include <stdint.h>
#include <SDL.h>
#include <SDL_ttf.h>

#define SCALE_X (1)
#define SCALE_Y (1)

#define NUM_FONTS (2)

struct render
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	uint8_t locked;
	uint32_t frame_tick;
};

struct FONT
{
	const char *path;
	TTF_Font *font;
	uint32_t size;
};

struct FONT _font_list[NUM_FONTS] =
{
	{"font\\Consolas.ttf", NULL, 20},
	{"font\\Consolas.ttf", NULL, 10}
};

#define FPS (60)

RENDER_S *render_setup()
{
	RENDER_S *render;
	uint32_t i;
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		return NULL;
	}
	if (TTF_Init() != 0)
	{
		return NULL;
	}
	render = malloc(sizeof(RENDER_S));
	if (render == NULL)
	{
		log_output("render: Insufficient memory\n");
		return NULL;
	}
	render->window = SDL_CreateWindow(PROGRAM_NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * SCALE_X, SCREEN_HEIGHT * SCALE_Y, SDL_WINDOW_ALLOW_HIGHDPI);
	render->renderer = SDL_CreateRenderer(render->window, -1, SDL_RENDERER_ACCELERATED);
	for (i = 0; i < NUM_FONTS; ++i)
	{
		_font_list[i].font = TTF_OpenFont(_font_list[i].path, _font_list[i].size);
	}
	SDL_RenderSetScale(render->renderer, SCALE_X, SCALE_Y);
	
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
	SDL_DestroyRenderer(render->renderer);
	SDL_DestroyWindow(render->window);
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
	SDL_RenderFillRect(render->renderer, &rect);
}

void render_draw_text(RENDER_S *render, int32_t x, int32_t y, const char *text, uint32_t font_number, uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t x_alignment, uint8_t y_alignment, uint8_t quality)
{
	SDL_Color color = {r, g, b, a};
	TEXTURE texture;
	SDL_Surface *text_surface = NULL;
	if (quality == QUALITY_BEST)
	{
		text_surface = TTF_RenderText_Blended(_font_list[font_number].font, text, color);
	}
	else
	{
		text_surface = TTF_RenderText_Solid(_font_list[font_number].font, text, color);
	}

	if (text_surface == NULL)
	{
		log_output("render: Could not create texture: %s\n", TTF_GetError());
		return;
	}
	SDL_Texture *temp = SDL_CreateTextureFromSurface(render->renderer, text_surface);
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
	
	int32_t x_mod;
	int32_t y_mod;

	if (x_alignment == ALIGN_RIGHT)
	{
		x_mod = texture.w;
	}
	else if (x_alignment == ALIGN_CENTER)
	{
		x_mod = texture.w / 2;
	}
	else
	{
		x_mod = 0;
	}

	if (y_alignment == ALIGN_BOTTOM)
	{
		y_mod = texture.h;
	}
	else if (y_alignment == ALIGN_CENTER)
	{
		y_mod = texture.h / 2;
	}
	else
	{
		y_mod = 0;
	}

	render_draw_texture(render, texture, x - x_mod, y - y_mod);

	render_delete_texture(texture);
}

void render_draw_texture(RENDER_S *render, TEXTURE texture, int32_t x, int32_t y)
{
	SDL_Rect src = {0, 0, texture.w, texture.h};
	SDL_Rect dst = {x, y, texture.w, texture.h};

	SDL_RenderCopy(render->renderer, texture.texture, &src, &dst);
}

void render_delete_texture(TEXTURE texture)
{
	SDL_DestroyTexture(texture.texture);
}

void render_begin_frame(RENDER_S *render)
{
	SDL_SetRenderDrawColor(render->renderer, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(render->renderer);
}

void render_end_frame(RENDER_S *render)
{
	SDL_RenderPresent(render->renderer);
	if (render->locked)
	{
		uint32_t elapsed_time = SDL_GetTicks() - render->frame_tick;
		if (elapsed_time < 1000.0 / FPS)
		{
			SDL_Delay((uint32_t)(1000.0 / FPS) - elapsed_time);
		}
		render->frame_tick = SDL_GetTicks();
	}
}
