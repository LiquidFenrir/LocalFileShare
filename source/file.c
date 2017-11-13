#include "file.h"

Result openFile(Handle * filehandle, const char * path, bool write)
{
	FS_Archive archive = (FS_Archive){ARCHIVE_SDMC};
	FS_Path emptyPath = fsMakePath(PATH_EMPTY, "");
	u32 flags = (write ? (FS_OPEN_CREATE | FS_OPEN_WRITE) : FS_OPEN_READ);
	FS_Path filePath = fsMakePath(PATH_ASCII, path);
	
	Result ret = FSUSER_OpenFileDirectly(filehandle, archive, emptyPath, filePath, flags, 0);
	if (R_FAILED(ret)) printf("Error in:\nFSUSER_OpenFileDirectly\nResult: 0x%.8lx\n", ret);
	//truncate the file to remove previous contents before writing, in case it already existed
	if (write) {
		ret = FSFILE_SetSize(*filehandle, 0);
		if (R_FAILED(ret)) printf("Error in:\nFSFILE_SetSize\nResult: 0x%.8lx\n", ret);
	}
	
	return ret;
}
