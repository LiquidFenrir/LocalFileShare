#pragma once

#include "common.h"

void print_constatus(void);

Result initComm(bool server);
void exitComm(bool server);

Result receiveData(void * data, size_t bufSize, size_t * receivedSize);

Result sendData(void * data, size_t size);
inline Result sendString(const char * str)
{
	return sendData((void *)str, strlen(str));
}
