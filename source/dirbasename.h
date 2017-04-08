#pragma once

#include "common.h"

char * dirbasename(char * path, bool base);

inline char * dirname(char * path)
{
    return dirbasename(path, false);
}

inline char * basename(char * path)
{
    return dirbasename(path, true);
}
