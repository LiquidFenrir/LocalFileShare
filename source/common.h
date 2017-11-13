#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <3ds.h>

extern PrintConsole debugConsole, uiConsole;

#define PASSWORD_MAX 64
extern char password[PASSWORD_MAX+1];
extern u32 passwordChecksum;

#define PACKET_DATA_SIZE 0x400 //1 KiB

enum COMM_MODE {
	MODE_CANCEL = -1,
	MODE_CLIENT = 0,
	MODE_SERVER = 1,
};

typedef struct {
	u32 size;
	u32 packetID;
	u32 checksum;
	u8 data[PACKET_DATA_SIZE];
} filePacket;
