#include "dirbasename.h"

static void trimTrailingSlashes(char * str)
{
    int lastcharpos = strlen(str)-1;
    while (str[lastcharpos] == 0x2F) {
        str[lastcharpos] = 0x0;
        lastcharpos--; //removed the last character
    }
}

char * dirbasename(char * path, bool base)
{
	///Follows description at https://linux.die.net/man/3/dirname

	///If path is a NULL pointer or points to an empty string, then both dirname() and basename() return the string ".".
	if (path == NULL || path[0] == 0x0) return ".";

	///If path is the string "/", then both dirname() and basename() return the string "/".
	if (strcmp(path, "/") == 0) return "/";

	///If path does not contain a slash, dirname() returns the string "." while basename() returns a copy of path.
	if (strchr(path, '/') == NULL) {
		if (base) {
			return strdup(path);
		} else {
			return ".";
		}
	}

	///Trailing '/' characters are not counted as part of the pathname.
	//path without trailing slashes, will get modified by strtok
	char * usepath = strdup(path);
	trimTrailingSlashes(usepath);
	//path without trailing slashes, doesn't get modified by strtok
	char * returnpath = strdup(usepath);

	//does the breaking
	char * token = "";
	char * oldtoken = "";

	token = strtok(usepath, "/");

	while (token != NULL) {
		oldtoken = token;
		token = strtok(NULL, "/");
	}

	if (base) {
		///basename() returns the component following the final '/'.
		returnpath = strdup(oldtoken);
	} else {
		///dirname() returns the string up to, but not including, the final '/'.
		returnpath[strlen(returnpath)-strlen(oldtoken)-1] = 0x0;
		if (returnpath[0] == 0x0) returnpath[0] = '/';
	}

	free(usepath);
	return returnpath;
}
