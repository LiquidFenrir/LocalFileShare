#include "ui.h"

#define PERCENTAGE_PER_CHAR 5
#define PERCENT_CHAR '#'

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
	for (i = 0; i < (int)(currentBlock*100/blocks); i += PERCENTAGE_PER_CHAR) putchar(PERCENT_CHAR);
	for (; i < 100; i += PERCENTAGE_PER_CHAR) printf(" ");
	printf("]");
	
	printf("\x1b[u");
}

void printInstructions(void)
{
	printf("Press START to quit.\n");
	printf("Press SELECT to show these instruction.\n");
	printf("Press X to send a file.\n");
	printf("Press Y to receive a file.\n");
}
