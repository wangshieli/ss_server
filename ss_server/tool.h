#pragma once
#include "ss_global_resource.h"

void myprintf(const char* lpFormat, ...);

void SS_TEXT(const char* lpFormat, ...);

void ErrorCode2Text(DWORD dwErr);

void ss_check_hb_timeout();

void _stdcall ss_hb_timer(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime);

void getguid(char *buf);

void ss_check_proxy_timeout();

void _stdcall ss_proxy_timer(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime);

void _stdcall ss_listen_timer(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime);

void ss_check_listen_timeout();

void ss_insertpendingacceptlist(PPER_HANDLE_DATA pClient);

void ss_deletependingacceptlist(PPER_HANDLE_DATA pClient);