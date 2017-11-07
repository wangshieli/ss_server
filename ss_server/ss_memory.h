#pragma once
#include "ss_global_resource.h"

PPER_HANDLE_DATA AllocHandleData(device_type dt);

void FreeHandleData(PPER_HANDLE_DATA pFree);

PPER_IO_DATA AllocIoData();

void FreeIoData(PPER_IO_DATA pFree);