#include "common.h"
#include "uds.h"
#include "ui.h"
#include "file.h"
#include "checksum.h"
#include "filebrowser/filebrowser.h"
#include "keyboard.h"
#include "dirbasename.h"

PrintConsole debugConsole, uiConsole;

#define handleCancel() hidScanInput(),((hidKeysDown() | hidKeysHeld()) & KEY_B)

Result sendFile(void)
{
	printf("Sending a file...\n");
	printf("Select the file you want to send.\n");
	
	consoleSelect(&uiConsole);
	char * filepath = filebrowser("/");
	consoleSelect(&debugConsole);
	
	if (filepath == NULL) {
		printf("Error when getting the path. Cancelled.\n");
		return -1;
	}
	
	printf("Sending file from:\n%s\n", filepath);
	char * filename = my_basename(filepath);
	sendString(filename);
	free(filename);
	
	Handle filehandle;
	Result ret = openFile(&filehandle, filepath, false);
	free(filepath);
	
	if(R_FAILED(ret)) {
		printf("Error in:\nopenFile\nResult: 0x%.8lx\n", ret);
		return ret;
	}
	
	u64 fileSize = 0;
	ret = FSFILE_GetSize(filehandle, &fileSize);
	if(R_FAILED(ret)) {
		printf("Error in:\nFSFILE_GetSize\nResult: 0x%.8lx\n", ret);
		goto exit;
	}
	
	printf("Size of the file to send:\n%llu bytes\n", fileSize);
	
	u32 lastBlockSize = fileSize%PACKET_DATA_SIZE;
	u32 blockCount = (fileSize-lastBlockSize)/PACKET_DATA_SIZE;
	if (lastBlockSize > 0) blockCount += 1;
	
	printf("Number of blocks to send:\n%lu\n", blockCount);
	sendData(&blockCount, sizeof(blockCount));
		
	size_t receivedSize = 0;
	filePacket packet = {0};
	
	u32 requestedBlock = 0;
	
	printf("Starting transfer, press B to abort...\n");
	while (true) {
		if(handleCancel()) goto cancel;
		
		//receive the id of the block to send
		do {
			receivedSize = 0;
			receiveData(&requestedBlock, sizeof(requestedBlock), &receivedSize);
			if(handleCancel()) goto cancel;
		} while(receivedSize == 0);
		
		//if the received data is "OK" followed by a nullbyte, the receiver is done and we can stop
		if(receivedSize == 3 && !strncmp((char*)&requestedBlock, "OK", 3)) break;
		else if(receivedSize == 3 && !strncmp((char*)&requestedBlock, "NO", 3)) goto cancel;
		
		packet.packetID = requestedBlock;
		readFile(filehandle, &packet.size, PACKET_DATA_SIZE * packet.packetID, packet.data, PACKET_DATA_SIZE);
		packet.checksum = getBufChecksum(packet.data, packet.size);
		
		sendData(&packet, sizeof(packet));
				
		progressBar(blockCount, requestedBlock);
	}
	
	if (receivedSize == 3 && !strncmp((char*)&requestedBlock, "OK", 3))
		printf("Done sending!");
	else {
		cancel:
		printf("Transfer aborted.\n");
		sendString("NO");
	}
	
	exit:
	closeFile(filehandle);
	return ret;
}

Result receiveFile(void)
{
	printf("Receiving a file...\n");
	printf("Select where you want to save the file.\n");
	
	consoleSelect(&uiConsole);
	char * savedir = filebrowser("/");
	consoleSelect(&debugConsole);
	
	if (savedir == NULL) {
		printf("Error when getting the path. Cancelled.\n");
		return -1;
	}
	
	size_t receivedSize = 0;
	filePacket packet = {0};
	
	printf("Waiting for the filename...\n");
	
	char * filename = (char*)packet.data;
	//get the file name and append that to the directory where it will be saved
	do {
		receivedSize = 0;
		receiveData(filename, PACKET_DATA_SIZE, &receivedSize);
		if(handleCancel()) goto cancel;
	} while(receivedSize == 0);
	
	char * filepath = calloc(strlen(savedir)+receivedSize, sizeof(char));
	strncpy(filepath, savedir, strlen(savedir)-2);
	sprintf(filepath, "%s/%s", filepath, filename);
	free(savedir);
	savedir = NULL;
	
	printf("Saving file to:\n%s\n", filepath);
	
	Handle filehandle;
	Result ret = openFile(&filehandle, filepath, true);
	
	free(filepath);
	
	if(R_FAILED(ret)) {
		printf("Error in:\nopenFile\nResult: 0x%.8lx\n", ret);
		return ret;
	}
	
	u32 blockCount = 0;
	do {
		receivedSize = 0;
		receiveData(&blockCount, sizeof(u32), &receivedSize);
		if(handleCancel()) goto cancel;
	} while(receivedSize == 0);
	printf("Number of blocks to receive:\n%lu\n", blockCount);
		
	u32 bytesWritten = 0;
	u32 i = 0;
		
	printf("Starting transfer, press B to abort...\n");
	for (i = 0; i < blockCount; i++) {
		request:
		if(handleCancel()) goto cancel;
		
		//send the id of the block to receive
		sendData(&i, sizeof(u32));
		
		//wait until you receive the packet
		do {
			receivedSize = 0;
			receiveData(&packet, sizeof(packet), &receivedSize);
			if(handleCancel()) goto cancel;
		} while(receivedSize == 0);
		
		//stop if the sender aborted
		if (receivedSize == 3 && strncmp((char*)&packet, "NO", 3) == 0) goto cancel;
		
		//check if the packet is the one requested
		//if it is, check if the data received got damaged during the transfer
		//if needed, request it again
		if (packet.packetID != i || getBufChecksum(packet.data, packet.size) != packet.checksum) {
			goto request;
		}
		
		//if all is good, write to the file, update the progress bar, and ask for the next block
		writeFile(filehandle, &bytesWritten, PACKET_DATA_SIZE * i, packet.data, packet.size);
		
		progressBar(blockCount, i);
	}
	
	if(i == blockCount) {
		printf("File completely received!\n");
		sendString("OK");
	}
	else {
		cancel:
		printf("Transfer aborted.\n");
		sendString("NO");
	}
	
	free(savedir);
	closeFile(filehandle);
	return 0;
}

int main()
{
	gfxInitDefault();
	udsInit(0x3000, NULL);
	
	consoleInit(GFX_TOP, &uiConsole);
	consoleInit(GFX_BOTTOM, &debugConsole);
	printf("Welcome to LocalFileShare!\nPlease type in the password to use\nand select what your console should be.\n");
	
	int mode = getMode(password, PASSWORD_MAX);
	passwordChecksum = getBufChecksum((u8*)password, PASSWORD_MAX);
	
	if (mode == MODE_CANCEL) goto exit;
	
	bool server = (bool)mode;
	Result ret = initComm(server);
	
	printf("\n");
	svcSleepThread(5e8); //sleep for half a second to give people time to see possible errors
	
	//if an error happened during the connection, just quit the application
	if (ret != 0) goto exit;
	
	printInstructions();
	
	while(aptMainLoop()) {
		hidScanInput();
		u32 kDown = hidKeysDown();
		
		if (kDown & KEY_START) break;
		else if (kDown & KEY_SELECT) printInstructions();
		else if (kDown & KEY_X) sendFile();
		else if (kDown & KEY_Y) receiveFile();
		
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}
	
	exitComm(server);
	
	exit:	
	udsExit();
	gfxExit();
	return 0;
}
