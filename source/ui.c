#include "ui.h"

#define PERCENTAGE_PER_CHAR 5
#define PERCENT_CHAR "#"

int display_menu(const char *menu_entries[], const int entries_amount, const char *headerstr, u32 allowed_buttons)
{
	bool redraw = true;
	u32 kDown;
	int ctr = 0;

	consoleClear();

	while (aptMainLoop()) {

		hidScanInput();
		kDown = hidKeysDown();

		if (redraw) {
			printf("\x1b[0;0H"); //places cursor at top left of screen
			printf("%s\n\n", headerstr);
			for (int i = 0; i < entries_amount; i++) {
				printf("%s %s\n", (ctr == i) ? "#" : " ", menu_entries[i]);
			}
			redraw = false;
		}
		
		if (kDown) redraw = true;
		
		if (kDown & KEY_A & allowed_buttons) return ctr;
		else if (kDown & (KEY_B | KEY_START) & allowed_buttons) break;
		else if (kDown & KEY_Y & allowed_buttons) return -2;
		else if (kDown & KEY_X & allowed_buttons) return -3;
		
		else if (kDown & KEY_LEFT) ctr = 0;
		else if (kDown & KEY_RIGHT) ctr = entries_amount-1;
		else if (kDown & KEY_DOWN) ctr++;
		else if (kDown & KEY_UP) ctr--;
		
		if (ctr >= entries_amount) ctr = 0;
		else if (ctr < 0) ctr = entries_amount-1;
		
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}
	
	return -1;
}

void progressBar(u32 blocks, u32 currentBlock)
{
	printf("\x1b[s");
	
	int i;
	char * blockStr = NULL;
	asprintf(&blockStr, "Block %lu of %lu", currentBlock, blocks);
	//erase the previous block count, to avoid having to clear the console (would cause blinking of the screen)
	for (i = 0; i <= (int)strlen(blockStr); i++) printf(" ");
	
	//start printing the actual info
	printf("\x1b[u");
	puts(blockStr);
	
	printf("[");
	for (i = 0; i < (int)(currentBlock*100/blocks); i += PERCENTAGE_PER_CHAR) printf(PERCENT_CHAR);
	for (; i < 100; i += PERCENTAGE_PER_CHAR) printf(" ");
	printf("]");
	
	printf("\x1b[u");
}
