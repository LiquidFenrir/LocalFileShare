#pragma once

#include "common.h"

int display_menu(const char *menu_entries[], const int entries_amount, const char *headerstr);

inline void instructions(void)
{
	puts("Press START to quit.\nPress Y to send a file.\nPress X to receive a file.\n");
}
