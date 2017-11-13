#include "uds.h"

#define APP_DATA_HEADER "3DS-LFS-"

char password[PASSWORD_MAX+1] = {0};
u32 passwordChecksum = 0;

static u32 wlancommID = 0x08426502;
#define APPDATA_SIZE (sizeof(APP_DATA_HEADER) + sizeof(passwordChecksum)*2)
static u8 appdata[4 + APPDATA_SIZE + 1] = {0x69, 0x8a, 0x05, 0x5c};
static char * appdataStr = (char*)appdata+4;

static u8 data_channel = 1;
static udsNetworkStruct networkstruct;
static udsBindContext bindctx;

static u32 recv_buffer_size = UDS_DEFAULT_RECVBUFSIZE;

static Result startServer(void)
{
	Result ret = 0;
	
	printf("Starting the server...\n");
	
	//only allow 2 nodes: the server and 1 client
	udsGenerateDefaultNetworkStruct(&networkstruct, wlancommID, 0, 2);
	
	ret = udsCreateNetwork(&networkstruct, password, strlen(password), &bindctx, data_channel, recv_buffer_size);
	if(R_FAILED(ret)) {
		printf("Error in:\nudsCreateNetwork\nResult: 0x%.8lx\n", ret);
		return ret;
	}
	
	ret = udsSetApplicationData(appdata, sizeof(appdata));
	if(R_FAILED(ret)) {
		printf("Error in:\nudsSetApplicationData\nResult: 0x%.8lx\n", ret);
		exitComm(true);
		return ret;
	}
	
	//dont allow spectators
	ret = udsEjectSpectator();
	if(R_FAILED(ret)) {
		printf("Error in:\nudsEjectSpectator\nResult: 0x%.8lx\n", ret);
		exitComm(true);
		return ret;
	}
	
	printf("Server started succesfully!\n");
	
	return ret;
}

static Result connectToServer(udsNetworkStruct network)
{
	Result ret = 0;
	
	printf("Connecting to the server as a client...\n");
	
	for(int pos = 0; pos < 10; pos++) { // 10 tries
		ret = udsConnectNetwork(&network, password, strlen(password), &bindctx, UDS_BROADCAST_NETWORKNODEID, UDSCONTYPE_Client, data_channel, recv_buffer_size);
		if(R_FAILED(ret)) {
			printf("udsConnectNetwork() returned 0x%.8lx\n", ret);
		}
		else {
			printf("Succesfully connected to the server!\n");
			break;
		}
		ret = -3;
	}
	
	return ret;
}

static Result startClient(void)
{
	Result ret = 0;
	
	printf("Starting client...\n");
	
	u32 tmpbuf_size = 0x4000;
	u32 * tmpbuf = calloc(tmpbuf_size, sizeof(u32));
	if(tmpbuf == NULL) {
		printf("Failed to allocate tmpbuf for beacon data.\n");
		return -1;
	}
		
	size_t total_networks = 0;
	udsNetworkScanInfo * networks = NULL;
	
	ret = udsScanBeacons(tmpbuf, tmpbuf_size, &networks, &total_networks, wlancommID, 0, NULL, false); //always returns -1 (failed to malloc networks) if there's a network to scan, 0 otherwise, but total_networks stays 0
	if(R_FAILED(ret)) {
		printf("udsScanBeacons() returned 0x%.8lx\n", ret);
	}
	
	if(total_networks > 0) {
		for(size_t selected_network = 0; selected_network < total_networks; selected_network++) {
			udsNetworkStruct network = networks[selected_network].network;
			if(memcmp(appdata, network.appdata, sizeof(appdata)) == 0 && network.max_nodes == 2 && network.total_nodes < 2) {
				ret = connectToServer(network);
				if(R_FAILED(ret)) {
					printf("Error: Couldn't connect to the server.\n");
				}
				break;
			}
		}
	}
	else {
		printf("Error: No servers found.\n");
		ret = -2;
	}
	
	free(networks);
	free(tmpbuf);
	
	return ret;
}

static void initAppData(void)
{
	printf("Initing AppData...\n");
	snprintf(appdataStr, APPDATA_SIZE, "%s%.8lx", APP_DATA_HEADER, passwordChecksum);
	printf("Using AppData: '%s'\n", appdataStr);
}

Result initComm(bool server)
{
	initAppData();
	return (server ? startServer() : startClient());
}

void exitComm(bool server)
{
	if (server)
		udsDestroyNetwork();
	else
		udsDisconnectNetwork();
	
	udsUnbind(&bindctx);
}

Result receiveData(void * data, size_t bufSize, size_t * receivedSize)
{
	u16 src_NetworkNodeID;
	Result ret = udsPullPacket(&bindctx, data, bufSize, receivedSize, &src_NetworkNodeID);
	if(R_FAILED(ret)) {
		printf("Error in:\nudsPullPacket\nResult: 0x%.8lx\n", ret);
		return ret;
	}
	
	return ret;
}

Result sendData(void * data, size_t size)
{
	Result ret = udsSendTo(UDS_BROADCAST_NETWORKNODEID, data_channel, UDS_SENDFLAG_Default, (u8 *)data, size);
	if(R_FAILED(ret)) {
		printf("Error in:\nudsPullPacket\nResult: 0x%.8lx\n", ret);
		return ret;
	}
	
	return ret;
}

Result sendPacket(filePacket * packet)
{
	return sendData((void*)packet, sizeof(filePacket));
}

Result sendString(const char * string)
{
	return sendData((void*)string, strlen(string));
}
