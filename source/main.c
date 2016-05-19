#include "system.h"
#include "game.h"
#include "log.h"
#include "menu.h"

#include <Windows.h>

#ifdef PLATFORM_RELEASE
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, char *argv[])
#endif
{
	MENU_S *menu;
	RENDER_S *render;
	if (log_setup() != 0)
	{
		return -1;
	}

	system_setup();
	if ((render = render_setup()) == NULL)
	{
		return -1;
	}
	if ((menu = menu_setup()) == NULL)
	{
		return -1;
	}
	game_loop(render, menu);
	menu_teardown(menu);
	render_teardown(render);
	system_teardown(system);

	if (log_teardown() != 0)
	{
		return -1;
	}
	return 0;
}