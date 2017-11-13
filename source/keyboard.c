#include "keyboard.h"

int getMode(char * password, size_t size)
{
	SwkbdState swkbd;
	SwkbdButton button = SWKBD_BUTTON_NONE;
	
	swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 3, -1);
	
	swkbdSetInitialText(&swkbd, password);
	swkbdSetHintText(&swkbd, "Password to use.");
	
	// swkbdSetFeatures(&swkbd, SWKBD_DARKEN_TOP_SCREEN);
	
	swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, "Cancel", false);
	swkbdSetButton(&swkbd, SWKBD_BUTTON_MIDDLE, "Client", true);
	swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, "Server", true);
	
	swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
	
	button = swkbdInputText(&swkbd, password, size);
	
	switch (button)
	{
		default:
		case SWKBD_BUTTON_NONE:
		case SWKBD_BUTTON_LEFT:
			return MODE_CANCEL;
		case SWKBD_BUTTON_MIDDLE:
			return MODE_CLIENT;
		case SWKBD_BUTTON_RIGHT:
			return MODE_SERVER;
	}
}
