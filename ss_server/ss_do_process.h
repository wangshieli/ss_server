#pragma once
#include "ss_global_resource.h"

BOOL ss_verify_main_link(PPER_HANDLE_DATA pPerHandle, PPER_IO_DATA pPerIoData, HANDLE hIocp);

BOOL ss_dispose_client_link(PPER_HANDLE_DATA pPerHandle, PPER_IO_DATA pPerIoData, HANDLE hIocp);

BOOL ss_dispose_redail_process(PPER_HANDLE_DATA pPerHandle, PPER_IO_DATA pPerIoData);

BOOL ss_dispose_ok_process(PPER_HANDLE_DATA pPerHandle, PPER_IO_DATA pPerIoData);

BOOL ss_dispose_hb_process(PPER_HANDLE_DATA pPerHandle, PPER_IO_DATA pPerIoData);

BOOL ss_dispose_hb_process2(PPER_HANDLE_DATA pPerHandle, PPER_IO_DATA pPerIoData);

BOOL ss_dispose_yesno_process(PPER_HANDLE_DATA pPerHandle, PPER_IO_DATA pPerIoData, BOOL bYes = TRUE);

BOOL ss_dispose_yesno_process1(PPER_HANDLE_DATA pPerHandle, PPER_IO_DATA pPerIoData);