#pragma once
#include "ss_global_resource.h"

typedef	void(*PFN_ON_GIOCP_ERROR)(void* pCompletionKey, void* pOverlapped);
typedef	void(*PFN_ON_GIOCP_OPER)(DWORD dwBytes, void* pCompletionKey, void* pOverlapped);

typedef struct _COMPLETION_KEY
{
PFN_ON_GIOCP_OPER			pfnOnIocpOper;
PFN_ON_GIOCP_ERROR			pfnOnIocpError;
}GIOCP_COMPLETION_KEY, *PGIOCP_COMPLETION_KEY;

unsigned int _stdcall ss_accept_thread(LPVOID pVoid);

unsigned int _stdcall ss_servo_threadMainLink(LPVOID pVoid);

unsigned int _stdcall ss_servo_threadRedailLink(LPVOID pVoid);

unsigned int _stdcall ss_servo_threadHBLink(LPVOID pVoid);