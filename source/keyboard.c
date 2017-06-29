#include "keyboard.h"

char * getStr(int max)
{
	char * str = calloc(max+1, sizeof(char));
	SwkbdState swkbd;
	swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 1, max);
	swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);
	swkbdInputText(&swkbd, str, max);
	return str;
}
