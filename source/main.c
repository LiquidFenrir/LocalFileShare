#include "common.h"
#include "uds.h"
#include "ui.h"
#include "file.h"
#include "filebrowser/filebrowser.h"
#include "keyboard.h"

PrintConsole debugConsole, uiConsole;

int main()
{
	gfxInitDefault();
	fsInit();
	udsInit(0x3000, "LFS");
	
	consoleInit(GFX_BOTTOM, &debugConsole);
	consoleInit(GFX_TOP, &uiConsole);
	consoleSelect(&uiConsole);
	
	const char * questions[] = {"Connect to a server", "Host a server"};
	int server = display_menu(questions, 2, "What do you want to do?\nPress B to exit.", KEY_A | KEY_B);
	if (server == -1) goto exit;
	
	consoleSelect(&debugConsole);
	Result ret = initComm(server);
	//if an error happened during the connection, just quit the application
	if (ret != 0) goto exit;
	
	printInstructions();
	
	while(aptMainLoop()) {
		hidScanInput();
		u32 kDown = hidKeysDown();
		
		if (kDown & KEY_START) break;
		else if (kDown & KEY_SELECT) printInstructions();
		else if (kDown & KEY_X) {
			printf("Sending a file...\n");
			printf("Select the file you want to send.\n");
			consoleSelect(&uiConsole);
			char * filepath = filebrowser();
			consoleSelect(&debugConsole);
			
			if (filepath == NULL) continue;
			
			printf("Sending file from:\n%s\n", filepath);
			char * filename = basename(filepath);
			sendString(filename);
			
			Handle filehandle;
			ret = openFile(filepath, &filehandle, false);
			if (R_FAILED(ret)) printf("openFile() returned 0x%08x.\n", (unsigned int)ret);
			free(filepath-3);
			
			u64 fileSize = 0;
			ret = FSFILE_GetSize(filehandle, &fileSize);
			if (R_FAILED(ret)) printf("FSFILE_GetSize() returned 0x%08x.\n", (unsigned int)ret);
			
			printf("Size of the file to send:\n%llu\n", fileSize);
			sendData(&fileSize, sizeof(fileSize));
			
			u32 lastBlockSize = fileSize%PACKET_DATA_SIZE;
			u32 blockCount = (fileSize-lastBlockSize)/PACKET_DATA_SIZE;
			blockCount += (lastBlockSize ? 1 : 0);
			
			printf("Number of blocks to send:\n%lu\n", blockCount);
			sendData(&blockCount, sizeof(blockCount));
			
			size_t receivedSize = 0;
			filePacket packet = {0};
			
			u32 requestedBlock = 0;
			u32 previousBlock = 1; //placeholder value, will get set to 0 afterwards
			u64 offset = 0;
			
			closeFile(filehandle);
			continue;
			
			printf("Starting sending, press B to abort...\n");
			while (true) {
				
				hidScanInput();
				if ((hidKeysDown() | hidKeysHeld()) & KEY_B) break;
				
				//receive the id of the block to send
				do {
					receiveData(&requestedBlock, sizeof(requestedBlock), &receivedSize);
					hidScanInput();
					if ((hidKeysDown() | hidKeysHeld()) & KEY_B) break;
				} while(receivedSize == 0);
				
				//if the received data is "OK" followed by a nullbyte, the receiver is done and we can stop
				if (receivedSize == 3 && !strncmp((char*)&requestedBlock, "OK", 3)) break;
				
				//if the requested block id increased, the data was received correctly and we can carry on
				if (requestedBlock != previousBlock) offset += packet.size;
				else {
					//if the requested id didn't change, send the same data, no need to read from the file again
					sendData(&packet, sizeof(packet));
					continue;
				}
				
				packet.packetID = requestedBlock;
				readFile(filehandle, &packet.size, offset, packet.data, PACKET_DATA_SIZE);
				packet.checksum = getBufChecksum(packet.data, packet.size);
				
				sendData(&packet, sizeof(packet));
				
				previousBlock = requestedBlock;
				
				progressBar(requestedBlock, blockCount);
			}
			printf("\n"); //to skip the progress bar
			
			if (receivedSize == 3 && !strncmp((char*)&requestedBlock, "OK", 3))
				printf("Done sending!");
			else {
				printf("Transfer aborted.\n");
				//tell the receiver we stop sending and he can abort
				sendString("NO");
			}
			
			closeFile(filehandle);
		}
		else if (kDown & KEY_Y) {
			printf("Receiving a file...\n");
			printf("Select where you want to save the file.\n");
			consoleSelect(&uiConsole);
			char * filepath = filebrowser();
			consoleSelect(&debugConsole);
			
			if (filepath == NULL) continue;
			
			size_t receivedSize = 0;
			filePacket packet = {0};
			
			//get the file name and append that to the directory where it will be saved
			do {
				receiveData(&packet.data, PACKET_DATA_SIZE, &receivedSize);
			} while(receivedSize == 0);
			
			realloc(filepath, strlen(filepath)+strlen((char*)packet.data)+1);
			printf("Saving file to:\n%s\n", filepath);
			
			Handle filehandle;
			ret = openFile(filepath, &filehandle, true);
			if (R_FAILED(ret)) printf("openFile() returned 0x%08x.\n", (unsigned int)ret);
			free(filepath-3);
			
			u32 blockCount = 0;
			do {
				receiveData(&blockCount, sizeof(u32), &receivedSize);
			} while(receivedSize == 0);
			printf("Number of blocks to receive:\n%lu\n", blockCount);
			
			u32 bytesWritten = 0;
			u64 offset = 0;
			u32 i = 0;
			
			closeFile(filehandle);
			continue;
			
			printf("Starting requests, press B to abort...\n");
			for (i = 0; i < blockCount; i++) {
				
				hidScanInput();
				if ((hidKeysDown() | hidKeysHeld()) & KEY_B) break;
				
				//send the id of the block to receive
				sendData(&i, sizeof(u32));
				
				//wait until you receive the packet
				do {
					receiveData(&packet, sizeof(packet), &receivedSize);
					hidScanInput();
					if ((hidKeysDown() | hidKeysHeld()) & KEY_B) break;
				} while(receivedSize == 0);
				
				//if the received data is "NO" followed by a nullbyte, the sender aborted
				if (receivedSize == 3 && !strncmp((char*)&packet, "NO", 3)) break;
				
				//check if the packet is the one requested
				//if it is, check if the data received got damaged during the transfer
				//if needed, request it again
				if (packet.packetID != i || getBufChecksum(packet.data, packet.size) != packet.checksum) {
					i -= 1;
					continue;
				}
				
				//if all is good, write to the file, update the progress bar, and ask for the next block
				writeFile(filehandle, &bytesWritten, offset, packet.data, packet.size);
				offset += packet.size;
				
				progressBar(i, blockCount);
			}
			printf("\n"); //to skip the progress bar
			
			//tell the sender to stop
			sendString("OK");
			
			if (i == blockCount) printf("File completely received!\n");
			else printf("Download aborted.\n");
			
			closeFile(filehandle);
		}
	}
	
	exitComm(server);
	
	exit:
	udsExit();
	fsExit();
	gfxExit();
	return 0;
}
