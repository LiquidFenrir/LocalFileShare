#pragma once

#include "common.h"

C2D_Text* makeText(const char* string);
inline C2D_Text* makeText(const std::string& string)
{
    return makeText(string.c_str());
}

void drawCentered(gfxScreen_t screen, C2D_Text* text, float y, float z, float scaleX, float scaleY, u32 color);
u32 getBufChecksum(u8 * buf, u32 size);
