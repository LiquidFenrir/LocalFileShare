#include "app/room.h"

std::shared_ptr<Room> currentRoom = nullptr;

static u32 wlancommID = 0xF660DA89;
#define appdataLen 0xc8
#define appdataSigLen 4
#define appdataStrLen (appdataLen-appdataSigLen)
static u8 appdata[appdataLen] = {0x69, 0x8a, 0x05, 0x5c};
#define appdataStr(ad) ((char*)ad+appdataSigLen)

#define usernameLength 0x1c

#define MAX_NODES_AMOUNT 2


static u8 data_channel = 1;
static u32 recv_buffer_size = UDS_DEFAULT_RECVBUFSIZE;

Room::Room()
{
    udsGenerateDefaultNetworkStruct(&this->network, wlancommID, 0, MAX_NODES_AMOUNT);

    u8 tmpUsername[usernameLength] = {0};
    Result ret = CFGU_GetConfigInfoBlk2(usernameLength, 0x000A0000, tmpUsername);
    if(ret != 0)
        DEBUG("udsSetApplicationData() returned %.8lx\n", ret);

    ret = udsCreateNetwork(&this->network, "", 0, &this->bindctx, data_channel, recv_buffer_size);
    if(ret != 0)
        DEBUG("udsCreateNetwork() returned %.8lx\n", ret);

    utf16_to_utf8((u8*)appdataStr(appdata), (u16*)tmpUsername, usernameLength/sizeof(u16));
    DEBUG("Appdata: %s\n", appdataStr(appdata));

    ret = udsSetApplicationData(appdata, appdataLen*sizeof(u8));
    if(ret != 0)
        DEBUG("udsSetApplicationData() returned %.8lx\n", ret);

    //dont allow spectators
    ret = udsEjectSpectator();
    if(ret != 0)
        DEBUG("udsEjectSpectator() returned %.8lx\n", ret);

    this->server = true;
    this->connected = true;
}

Room::Room(udsNetworkStruct network)
{
    this->network = network;

    this->server = false;
    this->connected = false;
}

Room::~Room()
{
    DEBUG("Room::~Room\n");

    if(this->connected)
    {
        if(this->server)
            udsDestroyNetwork();
        else
            udsDisconnectNetwork();

        udsUnbind(&this->bindctx);
    }
}

void Room::join()
{
    std::string password = "";
    for(int i = 0; i < 10; i++)  // 10 tries
    {
        Result ret = udsConnectNetwork(&this->network, password.c_str(), password.length(), &this->bindctx, UDS_BROADCAST_NETWORKNODEID, UDSCONTYPE_Client, data_channel, recv_buffer_size);
        if(R_FAILED(ret))
        {
            DEBUG("udsConnectNetwork() returned %.8lx\n", ret);
        }
        else
        {
            DEBUG("Succesfully connected to the room!\n");
            this->connected = true;
            break;
        }
    }
}

std::string Room::getName() const
{
    return std::string(appdataStr(this->network.appdata), this->network.appdata_size-appdataSigLen);
}

Result Room::receiveData(void* data, size_t size, size_t* actualSize)
{
    u16 sender;
    return udsPullPacket(&this->bindctx, data, size, actualSize, &sender);
}
Result Room::sendData(void* data, size_t size)
{
    return udsSendTo(UDS_BROADCAST_NETWORKNODEID, data_channel, UDS_SENDFLAG_Default, (u8*)data, size);
}

Result Room::receivePacket(NetworkPacket* packet, size_t* actualSize)
{
    return this->receiveData(packet, sizeof(NetworkPacket), actualSize);
}
Result Room::sendPacket(NetworkPacket packet)
{
    return this->sendData(&packet, sizeof(NetworkPacket));
}

bool Room::wait(Handle cancel)
{
    Handle events[2] = {
        this->bindctx.event,
        cancel,
    };
    s32 index = -1;
    svcWaitSynchronizationN(&index, events, 2, false, U64_MAX);
    svcClearEvent(this->bindctx.event);
    return index;
}

std::vector<std::shared_ptr<Room>> getRoomList()
{
    u32 tmpbuf_size = 0x4000;
    u32* tmpbuf = (u32*)malloc(tmpbuf_size);
    memset(tmpbuf, 0, tmpbuf_size);

    size_t total_networks = 0;
    udsNetworkScanInfo * networks = NULL;

    Result ret = udsScanBeacons(tmpbuf, tmpbuf_size, &networks, &total_networks, wlancommID, 0, NULL, false);
    if(R_FAILED(ret))
        DEBUG("udsScanBeacons() returned %.8lx\n", ret);

    DEBUG("Servers found: %u\n", total_networks);
    free(tmpbuf);

    std::vector<std::shared_ptr<Room>> rooms;
    for(size_t i = 0; i < total_networks; i++)
    {
        udsNetworkStruct network = networks[i].network;
        if(!memcmp(network.appdata, appdata, appdataSigLen) && network.max_nodes == MAX_NODES_AMOUNT && network.total_nodes < MAX_NODES_AMOUNT)
        {
            rooms.push_back(std::make_shared<Room>(network));
        }
    }

    free(networks);
    return rooms;
}
