#pragma once

#include "common.h"

#define PACKET_DATA_SIZE 0x400 //1 KiB

enum PacketType : u32 {
    PACKET_OK = 0,
    PACKET_METADATA,
    PACKET_DATA,
    PACKET_RESEND,
};

struct NetworkPacket {
    PacketType type;
    u32 size;
    u32 packetID;
    u32 checksum;
    u8 data[PACKET_DATA_SIZE];
};

class Room
{
    public:
        Room();
        Room(udsNetworkStruct network);
        ~Room();

        void join();

        std::string getName() const;

        Result receiveData(void* data, size_t size, size_t* actualSize);
        Result sendData(void* data, size_t size);
        
        Result receivePacket(NetworkPacket* packet, size_t* actualSize);
        Result sendPacket(NetworkPacket packet);
        
        bool wait(Handle cancel);  // returns true on cancel

    private:
        udsNetworkStruct network;
        udsBindContext bindctx;

        bool server;
        bool connected;
};

extern std::shared_ptr<Room> currentRoom;
std::vector<std::shared_ptr<Room>> getRoomList();