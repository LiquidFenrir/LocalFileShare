#pragma once

#include "common.h"

int display_menu(const char *menu_entries[], const int entries_amount, const char *headerstr);
void progressBar(u32 max, u32 current);

inline void instructions(void)
{
	puts("Press START to quit.\nPress Y to send a file.\nPress X to receive a file.\n");
}
