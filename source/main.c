#include "system.h"
#include "input.h"
#include "render.h"
#include "log.h"

#include <Windows.h>

#ifdef PLATFORM_RELEASE
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, char *argv[])
#endif
{
	system_setup();
	while (input_poll() == 0)
	{
		render_begin_frame();
		render_end_frame();
	}
	system_teardown();
	return 0;
}