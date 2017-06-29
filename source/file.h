#pragma once

#include "common.h"

u32 getBufChecksum(u8 * buf, u32 size);
Result openFile(const char * path, Handle * filehandle, bool write);

inline Result readFile(Handle filehandle, u32 * bytesRead, u64 offset, void * buffer, u32 size)
{
	return FSFILE_Read(filehandle, bytesRead, offset, buffer, size);
}
inline Result writeFile(Handle filehandle, u32 * bytesWritten, u64 offset, void * buffer, u32 size)
{
	return FSFILE_Write(filehandle, bytesWritten, offset, buffer, size, 0);
}
inline Result closeFile(Handle filehandle)
{
	return svcCloseHandle(filehandle);
}
