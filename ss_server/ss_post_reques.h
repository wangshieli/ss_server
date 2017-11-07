#pragma once
#include "ss_global_resource.h"

BOOL PostAccept(PPER_IO_DATA pAcceptIo, OP_TYPE ot);

BOOL PostAcceptEx(PPER_HANDLE_DATA pListener, device_type dt);

BOOL PostZeroByteRecv(PPER_HANDLE_DATA pPerHandleData, PPER_IO_DATA pPerIoData, OP_TYPE ot);

BOOL PostRecv(PPER_HANDLE_DATA pPerHandleData, PPER_IO_DATA pPerIoData, OP_TYPE ot);

BOOL PostZeroByteSend(PPER_HANDLE_DATA pPerHandleData, PPER_IO_DATA pPerIoData, OP_TYPE ot);

BOOL PostSend(PPER_HANDLE_DATA pPerHandleData, PPER_IO_DATA pPerIoData, OP_TYPE ot);