#include "common.h"
#include "dir.h"
#include "draw.h"
#include "sort.h"

static char currentPath[MAX_PATH_LEN+1]; //for the ending nullbyte

static dirInfo dirInfoArray[256];
static int dirCount = 0;
static int currentDir = 0;

char * filebrowser(void) {
	chdir("/");
	
	goto change;
	
	while (aptMainLoop()) {
		
		//things in there will only run if you do a goto
		//otherwise the screen would flicker because of constantly clearing
		if (false) {
			change:
			currentDir = 0;
			getcwd(currentPath, MAX_PATH_LEN+1);
			dirCount = listdir(currentPath, dirInfoArray);
			for (int i = dirCount; i < 256; i++) {
				dirInfoArray[i].name[0] = 0;
				dirInfoArray[i].isFile = false;
			}
			sortDirList(dirInfoArray, dirCount);
			
			draw:
			if (currentDir > dirCount-1) currentDir = dirCount-1;
			if (currentDir < 0) currentDir = 0;
			drawDirList(dirInfoArray, currentPath, currentDir, dirCount);
			gfxFlushBuffers();
			gfxSwapBuffers();
			gspWaitForVBlank();
		}
		
		hidScanInput();
		
		if (hidKeysDown() & KEY_START) {
			break;
		}
		else if (hidKeysDown() & KEY_A) {
			if (dirCount != 0) {
				if (dirInfoArray[currentDir].isFile) {
					//return the path to the file
					return strcat(currentPath, dirInfoArray[currentDir].name);
				}
				else {
					chdir(dirInfoArray[currentDir].name);
					goto change;
				}
			}
		}
		else if (hidKeysDown() & KEY_X) {
			return currentPath;
		}
		else if (hidKeysDown() & KEY_B) {
			chdir("..");
			goto change;
		}
		else if (hidKeysDown() & KEY_LEFT) {
			currentDir = 0;
			goto draw;
		}
		else if (hidKeysDown() & KEY_RIGHT) {
			currentDir = dirCount-1;
			goto draw;
		}
		else if (hidKeysDown() & KEY_UP) {
			currentDir--;
			goto draw;
		}
		else if (hidKeysDown() & KEY_DOWN) {
			currentDir++;
			goto draw;
		}
		
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}
	
	return "";
}
