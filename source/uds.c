// #include <zlib.h>
#include "uds.h"

// #define MAX_SIZE_FOR_SINGLE (2*1048576) //2 Mio
#define BLOCKSIZE 0x400 // 1 Kio, actual maximum is 0x5C6 but this is cleaner

static Result ret = 0;
static u32 pos;

static bool client = false;

static u32 wlancommID = 0x04114800;
static u8 appdata[0x14] = {0x69, 0x8a, 0x05, 0x5c};

static u8 data_channel = 1;
static udsNetworkStruct networkstruct;
static udsBindContext bindctx;

static u32 recv_buffer_size = UDS_DEFAULT_RECVBUFSIZE;
static u32 * tmpbuf;
static size_t tmpbuf_size;

static size_t actual_size = 0;
static u16 src_NetworkNodeID;

void print_constatus()
{
	udsConnectionStatus constatus;

	//By checking the output of udsGetConnectionStatus you can check for nodes (including the current one) which just (dis)connected, etc.
	ret = udsGetConnectionStatus(&constatus);
	if (R_FAILED(ret)) printf("udsGetConnectionStatus() returned 0x%08x.\n", (unsigned int)ret);
	else
	{
		printf("constatus:\nstatus=0x%x\n", (unsigned int)constatus.status);
		printf("1=0x%x\n", (unsigned int)constatus.unk_x4);
		printf("cur_NetworkNodeID=0x%x\n", (unsigned int)constatus.cur_NetworkNodeID);
		printf("unk_xa=0x%x\n", (unsigned int)constatus.unk_xa);
		for(pos = 0; pos < (0x20 >> 2); pos++) printf("%u=0x%x ", (unsigned int)pos+3, (unsigned int)constatus.unk_xc[pos]);
		printf("\ntotal_nodes=0x%x\n", (unsigned int)constatus.total_nodes);
		printf("max_nodes=0x%x\n", (unsigned int)constatus.max_nodes);
		printf("node_bitmask=0x%x\n", (unsigned int)constatus.total_nodes);
	}
}

void initCommID(void)
{
	psInit();
	u32 tempbuf;
	PS_GenerateRandomBytes(&tempbuf, 4);
	wlancommID ^= tempbuf;
	psExit();
}

char * getAppDataStr(udsNetworkScanInfo * network)
{
	char * retval = NULL;
	u8 out_appdata[0x14];
	
	ret = udsGetNetworkStructApplicationData(&network->network, out_appdata, sizeof(out_appdata), &actual_size);
	if (R_FAILED(ret) || (actual_size != sizeof(out_appdata)))
	{
		printf("udsGetNetworkStructApplicationData() returned 0x%08x. actual_size = 0x%x.\n", (unsigned int)ret, actual_size);
		return retval;
	}
				
	if (memcmp(out_appdata, appdata, 4) != 0)
	{
		puts("The first 4-bytes of appdata is invalid.\n");
		return retval;
	}

	return strdup((char*)&out_appdata[4]);
}

void waitForConnection(void)
{
	if (udsWaitConnectionStatusEvent(false, false))
	{
		printf("Constatus event signaled.\n");
		print_constatus();
	}
}

void startServer(void)
{	
	udsGenerateDefaultNetworkStruct(&networkstruct, wlancommID, 0, UDS_MAXNODES);

	puts("Creating the network...\n");
	
	puts("Type in the password you want for the network.\n");
	char * passphrase = getStr(64);
	printf("Password chosen: '%s'\n", passphrase);
	
	ret = udsCreateNetwork(&networkstruct, passphrase, strlen(passphrase)+1, &bindctx, data_channel, recv_buffer_size);
	if(R_FAILED(ret))
	{
		printf("udsstartServer() returned 0x%08x.\n", (unsigned int)ret);
		return;
	}
	free(passphrase);
	
	puts("Type in the name you want for the network.\n");
	char * name = getStr(10);
	printf("Name chosen: '%s'\n", name);
	strncpy((char*)&appdata[4], name, sizeof(appdata)-1);
	free(name);
	
	ret = udsSetApplicationData(appdata, sizeof(appdata));
	if(R_FAILED(ret))
	{
		printf("udsSetApplicationData() returned 0x%08x.\n", (unsigned int)ret);
		udsDestroyNetwork();
		udsUnbind(&bindctx);
		return;
	}

	u32 tmp = 0;
	ret = udsGetChannel((u8*)&tmp); //Normally you don't need to use this.
	printf("udsGetChannel() returned 0x%08x. channel = %u.\n", (unsigned int)ret, (unsigned int)tmp);
	if(R_FAILED(ret))
	{
		udsDestroyNetwork();
		udsUnbind(&bindctx);
		return;
	}
}

void connectToNetwork(udsNetworkScanInfo * network)
{
	u32 tmp = 0;
	
	printf("network: total nodes = %u.\n", (unsigned int)network->network.total_nodes);
	
	char * tmpstr = malloc(256);
	for (pos = 0; pos < UDS_MAXNODES; pos++)
	{
		if (!udsCheckNodeInfoInitialized(&network->nodes[pos])) continue;
		
		memset(tmpstr, 0, 256);
		
		ret = udsGetNodeInfoUsername(&network->nodes[pos], tmpstr);
		if (R_FAILED(ret))
		{
			printf("udsGetNodeInfoUsername() returned 0x%08x.\n", (unsigned int)ret);
			return;
		}
		
		printf("node%u username: %s\n", (unsigned int)pos, tmpstr);
	}
	free(tmpstr);
	
	tmpstr = getAppDataStr(network);
	printf("String from appdata: %s\n", tmpstr);
	free(tmpstr);
	
	puts("Connecting to the network as a client...\n");
	
	puts("Type in the password of the network.\n");
	char * passphrase = getStr(64);
	printf("Password used: '%s'\n", passphrase);
	
	for(pos=0; pos<10; pos++)
	{
		ret = udsConnectNetwork(&network->network, passphrase, strlen(passphrase)+1, &bindctx, UDS_BROADCAST_NETWORKNODEID, UDSCONTYPE_Client, data_channel, recv_buffer_size);
		if (R_FAILED(ret)) printf("udsConnectNetwork() returned 0x%08x.\n", (unsigned int)ret);
		else break;

		if (pos == 10) goto quit;

		printf("Connected.\n");

		tmp = 0;
		ret = udsGetChannel((u8*)&tmp);//Normally you don't need to use this.
		printf("udsGetChannel() returned 0x%08x. channel = %u.\n", (unsigned int)ret, (unsigned int)tmp);
		if (R_FAILED(ret)) goto quit;
	}
	
	tmpstr = getAppDataStr(network);
	printf("String from appdata: '%s'\n", tmpstr);
	free(tmpstr);

	client = true;
	
	quit:
	free(passphrase);
}

void startClient(void)
{
	udsNetworkScanInfo *networks = NULL;
	size_t total_networks = 0;
	
	tmpbuf_size = 0x4000;
	tmpbuf = malloc(tmpbuf_size);
	if (tmpbuf == NULL)
	{
		puts("Failed to allocate tmpbuf for beacon data.\n");
		return;
	}
	
	u32 i;
	
	scan:
	total_networks = 0;
	memset(tmpbuf, 0, sizeof(tmpbuf_size));
	ret = udsScanBeacons(tmpbuf, tmpbuf_size, &networks, &total_networks, wlancommID, 0, NULL, false);
	printf("udsScanBeacons() returned 0x%08x.\ntotal_networks=%u.\n", (unsigned int)ret, (unsigned int)total_networks);
	
	if (total_networks)
	{
		char * names[total_networks];
		for (i = 0; i < total_networks; i++) {
			names[i] = getAppDataStr(&networks[i]);
			if (names[i] == NULL) goto quit;
		}
		
		consoleSelect(&uiConsole);
		int selected = display_menu((const char **)names, total_networks, "Select a network to connect to or press Y to refresh:");
		consoleClear();
		consoleSelect(&debugConsole);
		
		for (i = 0; i < total_networks; i++) {
			free(names[i]);
		}
		
		if (selected == -1) goto quit;
		if (selected == -2 || selected == -3) goto scan;
		
		printf("Connecting to network #%u\n", selected);
		connectToNetwork(&networks[selected]);
		goto quit;
	}
	else
	{
		const char * refreshStr[] = {"No networks found."};
		consoleSelect(&uiConsole);
		int selected = display_menu(refreshStr, 1, "Select a network to connect to or press Y to refresh:");
		consoleClear();
		consoleSelect(&debugConsole);
		if (selected != -1) goto scan;
		goto quit;
	}
	
	quit:
	free(networks);
	return;
}

Result wait(bool udswait)
{
	if (udswait) udsWaitDataAvailable(&bindctx, false, true); //Check whether data is available via udsPullPacket().
	for (int i = 0; i < 10; i++)
	{
		memset(tmpbuf, 0, tmpbuf_size);
		actual_size = 0;
		src_NetworkNodeID = 0;
		ret = udsPullPacket(&bindctx, tmpbuf, tmpbuf_size, &actual_size, &src_NetworkNodeID);
		if(R_FAILED(ret))
		{
			printf("udsPullPacket() returned 0x%08x.\n", (unsigned int)ret);
			break;
		}
		if (actual_size != 0) break;
	}
	return ret;
}

Result sendData(const void * data, u32 size)
{
	ret = udsSendTo(UDS_BROADCAST_NETWORKNODEID, data_channel, UDS_SENDFLAG_Default, (u8 *)data, size);
	if (R_FAILED(ret)) printf("udsPullPacket() returned 0x%08x.\n", (unsigned int)ret);
	return ret;
}

Result sendStr(char * str)
{
	printf("Sending string:\n'%s'\n", str);
	return sendData(str, strlen(str)+1);
}

void sendFile(void)
{
	puts("Select a file to send\n");
	consoleSelect(&uiConsole);
	char * path = filebrowser();
	consoleClear();
	consoleSelect(&debugConsole);
	
	if (path[0] == 0x0) return;
	
	FILE * fh;
	u8 * tempbuf = NULL;
	
	fh = fopen(path, "rb");
	fseek(fh, 0, SEEK_END);
	u32 size = ftell(fh);
	fseek(fh, 0, SEEK_SET);
	
	sendStr("SENDING FILE");
	char * name = basename(path); 
	sendStr(name);
	free(name);
	sendData(&size, 4);
	//wait for the receiver to have selected where he wants to save the file
	if (wait(true)) goto quit;
	if (tmpbuf[0] != 0) goto quit;
	
	puts("Sending file\n");
	
	u16 lastblock = size%BLOCKSIZE;
	u16 blocksamount = (size - lastblock)/BLOCKSIZE;
	sendData(&blocksamount, 2);
	sendData(&lastblock, 2);
	printf("Sending %u blocks, lastblock size %u\n", blocksamount, lastblock);
	
	tempbuf = malloc(BLOCKSIZE);
	u16 oldblock = 0;
	u16 block = 0;
	u16 used_size = BLOCKSIZE;
	
	if (wait(true)) goto quit; //waiting for the OK
	
	while (block != blocksamount)
	{
		puts("Waiting for block request");
		if (wait(false)) goto quit;
		if (actual_size == 0) continue;
		block = tmpbuf[0];
		printf("block requested: %u of %u\n", block, blocksamount);
		if (block != (oldblock+1)) fseek(fh, BLOCKSIZE*block, SEEK_SET);
		if (block == blocksamount) used_size = lastblock;
		
		puts("Sending block.");
		fread(tempbuf, 1, used_size, fh);
		sendData(tempbuf, used_size);
	}
	free(tempbuf);
	
	puts("Done sending file.\n");
	
	quit:
	fclose(fh);
}

void receiveFile(void)
{
	puts("Waiting for a file to receive...\n");
	if (wait(true)) return;
	
	if (strcmp((char *)tmpbuf, "SENDING FILE") != 0) return;
	if (wait(true)) return;
	char * name = strdup((char *)tmpbuf);
	if (wait(true)) return;
	u32 size = tmpbuf[0];
	
	printf("Where do you want to save file %s ?\nFile size: %lu\n", name, size);
	printf("Press X in the folder you want to save the file to.\n");
	consoleSelect(&uiConsole);
	char * path = filebrowser();
	consoleClear();
	consoleSelect(&debugConsole);
	
	u32 val = 0;
	if (path[0] == 0x0) //if user quit instead of selecting a folder, path is empty
	{
		val++;
		sendData(&val, 4);
		return;
	}
	
	path = strcat(path, name);
	printf("Saving file to %s\n", path);
	FILE * fh = fopen(path, "wb");
	
	sendData(&val, 4);
	
	puts("Receiving file.\n");
	
	if (wait(true)) goto quit;
	u16 blocksamount = (u16 )tmpbuf[0];
	if (wait(true)) goto quit;
	u16 lastblock = (u16 )tmpbuf[0];
	printf("Receiving %u blocks, lastblock size %u\n", blocksamount, lastblock);
	
	sendData(&val, 4); //sending the OK
	
	for (u16 i = 0; i <= blocksamount; i++)
	{
		printf("Requesting block %u of %u\n", i, blocksamount);
		sendData(&i, 2); 
		// puts("Receiving block");
		if (wait(false)) goto quit;
		if (actual_size == 0)
		{
			i--;
			continue;
		}
		fwrite(tmpbuf, 1, actual_size, fh);
	}
	
	puts("Done receiving file.\n");
	
	quit:
	fclose(fh);	
}

void quitNetwork(void)
{
	free(tmpbuf);
	
	if (client) udsDestroyNetwork();
	else udsDisconnectNetwork();
	
	udsUnbind(&bindctx);
}

void startComm(bool server)
{
	initCommID();
	
	if (server) startServer();
	else startClient();
	
	waitForConnection();

	tmpbuf_size = UDS_DATAFRAME_MAXSIZE; // 0x5C6 bytes
	tmpbuf = malloc(tmpbuf_size);
	
	puts("Press START to quit.\nPress Y to send a file.\nPress X to receive a file.\n");
	
	while (true)
	{
		gspWaitForVBlank();
		hidScanInput();
		u32 kDown = hidKeysDown();
		
		if (kDown & KEY_START) break;
		else if (kDown & KEY_X) receiveFile();
		else if (kDown & KEY_Y) sendFile();
		else if (kDown & KEY_A) print_constatus();
		
		waitForConnection();
	}
	
	quitNetwork();
}
