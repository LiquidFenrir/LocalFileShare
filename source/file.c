#include "file.h"

u32 getBufChecksum(u8 * buf, u32 size)
{
	unsigned int i, j;
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

Result openFile(const char * path, Handle * filehandle, bool write)
{
	FS_Archive archive = (FS_Archive){ARCHIVE_SDMC};
	FS_Path emptyPath = fsMakePath(PATH_EMPTY, "");
	u32 flags = (write ? (FS_OPEN_CREATE | FS_OPEN_WRITE) : FS_OPEN_READ);
	FS_Path filePath = fsMakePath(PATH_ASCII, path);
	
	Result ret = FSUSER_OpenFileDirectly(filehandle, archive, emptyPath, filePath, flags, 0);
	if (R_FAILED(ret)) printf("Error in:\nFSUSER_OpenFileDirectly\nError: 0x%08x\n", (unsigned int)ret);
	
	return ret;
}
