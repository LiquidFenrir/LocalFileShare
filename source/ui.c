#include "ui.h"

#define PERCENTAGE_PER_CHAR 5
#define PERCENT_CHAR '#'

void progressBar(u32 blocks, u32 currentBlock)
{
	printf("\x1b[u\x1b[KBlock %lu of %lu", currentBlock, blocks);
}

void printInstructions(void)
{
	printf("Press START to quit.\n");
	printf("Press SELECT to show these instruction.\n");
	printf("Press X to send a file.\n");
	printf("Press Y to receive a file.\n");
}
