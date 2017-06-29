#pragma once

#include "common.h"

int display_menu(const char *menu_entries[], const int entries_amount, const char *headerstr, u32 allowed_buttons);
void progressBar(u32 blocks, u32 currentBlock);

inline void printInstructions(void) {
	printf("Press START to quit.\n");
	printf("Press SELECT to show these instruction.\n");
	printf("Press X to send a file.\n");
	printf("Press Y to receive a file.\n");
}