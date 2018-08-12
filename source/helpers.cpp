#include "helpers.h"

C2D_Text* makeText(const char* string)
{
    static C2D_Text c2dText;
    C2D_TextParse(&c2dText, dynamicBuf, string);
    C2D_TextOptimize(&c2dText);
    return &c2dText;
}

void drawCentered(gfxScreen_t screen, C2D_Text* text, float y, float z, float scaleX, float scaleY, u32 color)
{
    float width = 0;
    C2D_TextGetDimensions(text, scaleX, scaleY, &width, NULL);
    float offset = ((screen == GFX_TOP ? 400 : 320) - width)/2;
    C2D_DrawText(text, C2D_WithColor, offset, y, z, scaleX, scaleY, color);
}

u32 getBufChecksum(u8 * buf, u32 size)
{
	u32 i, j;
	u32 byte, crc, mask;
	crc = 0xFFFFFFFF;
	
	for (i = 0; i < size; i++) {
		byte = buf[i];
		crc = crc ^ byte;
		for (j = 0; j < 8; j++) {
			mask = -(crc & 1);
			crc = (crc >> 1) ^ (0xEDB88320 & mask);
		}
	}
	
	return ~crc;
}
