#include "system.h"
#include "game.h"
#include "log.h"

#include <Windows.h>

#ifdef PLATFORM_RELEASE
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, char *argv[])
#endif
{
	if (log_setup() != 0)
	{
		return -1;
	}

	system_setup();
	
	game_loop();
	system_teardown();

	if (log_teardown() != 0)
	{
		return -1;
	}
	return 0;
}