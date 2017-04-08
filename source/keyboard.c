#include "keyboard.h"

char * getStr(int max)
{
	char * str = malloc(max);
	memset(str, 0, max);
	static SwkbdState swkbd;
	swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, max);
	swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);
	swkbdInputText(&swkbd, str, max);
	return str;
}
