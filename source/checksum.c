#include "checksum.h"

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
