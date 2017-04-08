#include <dirent.h>
#include "dir.h"

int listdir(char * path, dirInfo * dirInfoArray)
{
	DIR *dir;
	struct dirent *ent;
	int count = 0;
	
	if ((dir = opendir(path)) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			strcpy(dirInfoArray[count].name, ent->d_name);
			dirInfoArray[count].isFile = (ent->d_type == 8);
			count++;
		}
		closedir (dir);
	}
	
	return count;
}
