#pragma once
#include "ss_global_resource.h"

BOOL InitializeWinsock2();

BOOL InitializeListenSocket(PPER_HANDLE_DATA pPerHandleData, HANDLE hIocp, const char* g_Port);

void LSCloseSocket(PPER_IO_DATA pPerIoData);

void LSCloseSocket_cs(PPER_HANDLE_DATA pPerHandleData);

BOOL GetAcceptexAndGetAcceptexSockAddr();