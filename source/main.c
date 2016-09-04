#include "system.h"
#include "game.h"
#include "log.h"
#include "menu.h"

#include <Windows.h>
#include <crtdbg.h>

#ifdef PLATFORM_RELEASE
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, char *argv[])
#endif
{
	MENU_S *menu;
	_CrtSetDbgFlag(_CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_CRT_DF | _CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_CHECK_EVERY_16_DF);
#ifndef PLATFORM_RELEASE
	int i;
	for (i = 0; i < argc; i++)
	{
		log_output(argv[i]);
	}
#endif
	if (log_setup() != 0)
	{
		return -1;
	}

	system_setup();
	render_setup();
	if ((menu = menu_setup()) == NULL)
	{
		return -1;
	}
	game_loop(menu);
	menu_teardown(menu);
	render_teardown();
	system_teardown();

	if (log_teardown() != 0)
	{
		return -1;
	}
	return 0;
}