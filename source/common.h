#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <3ds.h>

extern PrintConsole debugConsole, uiConsole;

#define PACKET_DATA_SIZE 0x400 //1 KiB

typedef struct {
	u32 size;
	u32 packetID;
	u32 checksum;
	u8 data[PACKET_DATA_SIZE];
} filePacket;
