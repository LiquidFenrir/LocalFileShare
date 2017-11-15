#pragma once

#include "common.h"

Result initComm(bool server);
void exitComm(bool server);

Result receiveData(void * data, size_t bufSize, size_t * receivedSize);
Result sendData(void * data, size_t size);
Result sendPacket(filePacket packet);
Result sendString(const char * string);
