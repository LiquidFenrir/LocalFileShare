#include "uds.h"

PrintConsole debugConsole, uiConsole;

int main()
{
	gfxInitDefault();
	udsInit(0x3000, NULL);
	
	consoleInit(GFX_BOTTOM, &debugConsole);
	consoleInit(GFX_TOP, &uiConsole);
	consoleSelect(&uiConsole);
	
	const char * serverquestions[] = {"Connect to server", "Host server"};
	int selected = display_menu(serverquestions, 2, "What do you want to do?");
	consoleClear();
	consoleSelect(&debugConsole);
	
	if (selected == -1) goto exit;
	startComm(selected);
	
	exit:
	udsExit();
	gfxExit();
	return 0;
}
