#include "uds.h"

#include "ui.h"
#include "keyboard.h"

static u32 wlancommID = 0x04114800;
static u8 appdata[0x14] = {0x69, 0x8a, 0x05, 0x5c};

static u8 data_channel = 1;
static udsNetworkStruct networkstruct;
static udsBindContext bindctx;

static u32 recv_buffer_size = UDS_DEFAULT_RECVBUFSIZE;

void print_constatus(void)
{
	Result ret = 0;
	u32 pos;
	udsConnectionStatus constatus;

	//By checking the output of udsGetConnectionStatus you can check for nodes (including the current one) which just (dis)connected, etc.
	ret = udsGetConnectionStatus(&constatus);
	if(R_FAILED(ret)) {
		printf("udsGetConnectionStatus() returned 0x%08x.\n", (unsigned int)ret);
	}
	else {
		printf("constatus:\nstatus=0x%x\n", (unsigned int)constatus.status);
		printf("1=0x%x\n", (unsigned int)constatus.unk_x4);
		printf("cur_NetworkNodeID=0x%x\n", (unsigned int)constatus.cur_NetworkNodeID);
		printf("unk_xa=0x%x\n", (unsigned int)constatus.unk_xa);
		for (pos = 0; pos < 8; pos++)
			printf("%u=0x%x ", (unsigned int)pos+3, (unsigned int)constatus.unk_xc[pos]);
		printf("\ntotal_nodes=0x%x\n", (unsigned int)constatus.total_nodes);
		printf("max_nodes=0x%x\n", (unsigned int)constatus.max_nodes);
		printf("node_bitmask=0x%x\n", (unsigned int)constatus.total_nodes);
	}
}

static char * getNetworkAppDataStr(udsNetworkScanInfo * network)
{
	Result ret = 0;
	u8 out_appdata[0x14] = {0};
	size_t actual_size;
	
	ret = udsGetNetworkStructApplicationData(&network->network, out_appdata, sizeof(out_appdata), &actual_size);
	if (R_FAILED(ret) || (actual_size != sizeof(out_appdata))) {
		printf("udsGetNetworkStructApplicationData() returned 0x%08x. actual_size = 0x%x.\n", (unsigned int)ret, actual_size);
		return NULL;
	}
				
	if (memcmp(out_appdata, appdata, 4) != 0) {
		printf("The first 4 bytes of network appdata is invalid.\n");
		return NULL;
	}

	return strdup((char*)(out_appdata+4));
}

static Result startServer(void)
{
	Result ret = 0;
	
	printf("Starting the server...\n");
	
	udsGenerateDefaultNetworkStruct(&networkstruct, wlancommID, 0, UDS_MAXNODES);
	
	printf("Type in the password for the server.\n");
	char * passphrase = getStr(64);
	printf("Password chosen:\n'%s'\n", passphrase);
	
	ret = udsCreateNetwork(&networkstruct, passphrase, strlen(passphrase)+1, &bindctx, data_channel, recv_buffer_size);
	if(R_FAILED(ret)) {
		printf("udsCreateNetwork() returned 0x%08x.\n", (unsigned int)ret);
		return ret;
	}
	free(passphrase);
	
	printf("Type in the name for the server.\n");
	char * name = getStr(10);
	printf("Name chosen:\n'%s'\n", name);
	strncpy((char*)(appdata+4), name, 10);
	free(name);
	
	ret = udsSetApplicationData(appdata, sizeof(appdata));
	if(R_FAILED(ret)) {
		printf("udsSetApplicationData() returned 0x%08x.\n", (unsigned int)ret);
		exitComm(true);
		return ret;
	}
	
	printf("Server started succesfully!\n");
	
	return 0;
}

static Result connectToServer(udsNetworkScanInfo * network)
{
	Result ret = 0;
	int pos;
	
	// printf("network: total nodes = %u.\n", (unsigned int)network->network.total_nodes);
	
	// char * tmpstr = malloc(256);
	// for (pos = 0; pos < UDS_MAXNODES; pos++) {
		// if (!udsCheckNodeInfoInitialized(&network->nodes[pos])) continue;
		
		// memset(tmpstr, 0, 256);
		
		// ret = udsGetNodeInfoUsername(&network->nodes[pos], tmpstr);
		// if (R_FAILED(ret)) {
			// printf("udsGetNodeInfoUsername() returned 0x%08x.\n", (unsigned int)ret);
			// return ret;
		// }
		
		// printf("node%u username: %s\n", (unsigned int)pos, tmpstr);
	// }
	// free(tmpstr);
	
	printf("Connecting to the server as a client...\n");
	
	printf("Type in the password of the server.\n");
	char * passphrase = getStr(64);
	printf("Password used:\n'%s'\n", passphrase);
	
	for (pos = 0; pos < 10; pos++) { // 10 tries
		ret = udsConnectNetwork(&network->network, passphrase, strlen(passphrase)+1, &bindctx, UDS_BROADCAST_NETWORKNODEID, UDSCONTYPE_Client, data_channel, recv_buffer_size);
		if (R_FAILED(ret)) printf("udsConnectNetwork() returned 0x%08x.\n", (unsigned int)ret);
		else {
			printf("Succesfully connected to the server!\n");
		}
	}
	free(passphrase);
	
	return ret;
}

static Result startClient(void)
{
	Result ret = 0;
	udsNetworkScanInfo * networks = NULL;
	size_t total_networks;
	unsigned int i;
	
	size_t tmpbuf_size = 0x4000;
	u32 * tmpbuf = malloc(tmpbuf_size);
	if (tmpbuf == NULL) {
		printf("Failed to allocate tmpbuf for beacon data.\n");
		return 1;
	}
	
	scan:
	total_networks = 0;
	memset(tmpbuf, 0, tmpbuf_size*sizeof(u32));
	ret = udsScanBeacons(tmpbuf, tmpbuf_size, &networks, &total_networks, wlancommID, 0, NULL, false);
	if (R_FAILED(ret)) printf("udsScanBeacons() returned 0x%08x.\n", (unsigned int)ret);
	
	if (total_networks) {
		printf("%u server found\n", total_networks);
		
		char ** names = malloc(total_networks*sizeof(char*));
		for (i = 0; i < total_networks; i++) {
			names[i] = getNetworkAppDataStr(&networks[i]);
		}
		
		const char * headerstr = "Select the server you want to connect to.\nPress Y to rescan if you don't see it.\nPress B to abort.";
		consoleSelect(&uiConsole);
		int selected_network = display_menu((const char **)names, total_networks, headerstr, KEY_A | KEY_B | KEY_Y);
		consoleClear();
		consoleSelect(&debugConsole);
		
		if (selected_network >= 0) {
			printf("Connecting to server %u, named\n%s\n", selected_network, names[i]);
			ret = connectToServer(&networks[selected_network]);
		}
		
		for (i = 0; i < total_networks; i++) {
			free(names[i]);
		}
		free(names);
		
		if (selected_network == -1) goto quit;
		if (selected_network == -2) goto scan;
	}
	else {
		printf("No server found, scanning again in 3 seconds...\n");
		printf("Press B to abort...\n");
		u64 startTime = osGetTime();
		while (osGetTime() < (startTime + 3000)) {
			hidScanInput();
			if ((hidKeysDown() | hidKeysHeld()) & KEY_B) {
				ret = 2;
				goto quit;
			}
		}
		goto scan;
	}
	
	quit:
	free(networks);
	
	return ret;
}

static void initCommID(void)
{
	psInit();
	u32 tempbuf;
	PS_GenerateRandomBytes(&tempbuf, sizeof(tempbuf));
	wlancommID ^= tempbuf;
	psExit();
}

Result initComm(bool server)
{
	initCommID();
	return (server ? startServer() : startClient());
}

void exitComm(bool server)
{
	if (server) udsDisconnectNetwork();
	else udsDestroyNetwork();
	
	udsUnbind(&bindctx);
}

Result receiveData(void * data, size_t bufSize, size_t * receivedSize)
{
	u16 src_NetworkNodeID;
	Result ret = udsPullPacket(&bindctx, data, bufSize, receivedSize, &src_NetworkNodeID);
	if (R_FAILED(ret)) printf("udsPullPacket() returned 0x%08x.\n", (unsigned int)ret);
	return ret;
}

Result sendData(void * data, size_t size)
{
	Result ret = udsSendTo(UDS_BROADCAST_NETWORKNODEID, data_channel, UDS_SENDFLAG_Default, (u8 *)data, size);
	if (R_FAILED(ret)) printf("udsSendTo() returned 0x%08x.\n", (unsigned int)ret);
	return ret;
}
