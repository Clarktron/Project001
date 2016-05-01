#include "input.h"
#include "render.h"

#include <SDL.h>

int32_t input_poll()
{
	SDL_Event ev;
	while (SDL_PollEvent(&ev))
	{
		switch (ev.type)
		{
			case SDL_QUIT:
				return 1;
		}
	}
	return 0;
}