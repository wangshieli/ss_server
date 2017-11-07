#include "ss_accept_thread.h"
#include "ss_post_reques.h"
#include "ss_init_socket.h"
#include "tool.h"

void GTcpSvr_DoCloseClient(PPER_HANDLE_DATA pClient, PPER_HANDLE_DATA pIoDataOwner)
{
	PPER_IO_DATA pIoData = PPER_IO_DATA(pIoDataOwner->pAcceptIo);
	if(pIoData)
	{
		CancelIoEx((HANDLE)pClient->s_LinkSock, &pIoData->ol);
	}else
	{
		myprintf("¼àÌý¶ÔÏó´íÎó");
	}
}

unsigned int _stdcall ss_servo_threadMainLink(LPVOID pVoid)
{
	int nIndex = 0;
	while (true)
	{
		nIndex = WSAWaitForMultipleEvents(1, &g_hSvrListenerEvent[0], FALSE, WSA_INFINITE, FALSE);
		if (WSA_WAIT_TIMEOUT != nIndex)
		{
			nIndex -= WSA_WAIT_EVENT_0;
			for (int i = 0; i < SS_MAX_MAIN_ACCEPT; i++)
			{
				if(!PostAcceptEx(g_hSvrListener[0], DT_MAIN_LINK))
					break;
			}
		}
	}

	return 0;
}

unsigned int _stdcall ss_servo_threadRedailLink(LPVOID pVoid)
{
	int nIndex = 0;
	while (true)
	{
		nIndex = WSAWaitForMultipleEvents(1, &g_hSvrListenerEvent[1], FALSE, WSA_INFINITE, FALSE);
		if (WSA_WAIT_TIMEOUT != nIndex)
		{
			nIndex -= WSA_WAIT_EVENT_0;
			for (int i = 0; i < SS_MAX_MAIN_ACCEPT; i++)
			{
				if(!PostAcceptEx(g_hSvrListener[1], DT_REDAIL_LINK))
					break;
			}
		}
	}

	return 0;
}

unsigned int _stdcall ss_servo_threadHBLink(LPVOID pVoid)
{
	int nIndex = 0;
	while (true)
	{
		nIndex = WSAWaitForMultipleEvents(1, &g_hSvrListenerEvent[2], FALSE, 1000, FALSE);
		if (WSA_WAIT_TIMEOUT != nIndex)
		{
			for (int i = 0; i < SS_MAX_MAIN_ACCEPT; i++)
			{
				if(!PostAcceptEx(g_hSvrListener[2], DT_HB_LINK))
					break;
			}
		}

#ifdef USE_DOS_DAVE
		INT nResult;
		INT nLong;
		INT nLongBytes = sizeof(INT);
		AcceptOutT.clear();
		EnterCriticalSection(&GTcpSvrPendingAcceptCS);
		PPER_HANDLE_DATA pClient = pGTcpSvrPendingAcceptHead;
		DWORD pGTcpSvrOvertimeCount = 0;
		while(pClient)
		{
			nResult = getsockopt(pClient->s_LinkSock, SOL_SOCKET, SO_CONNECT_TIME, (char *)&nLong, (PINT)&nLongBytes);
			if((SOCKET_ERROR != nResult) && (0xFFFFFFFF != nLong) && (5 < (DWORD)nLong))
			{
				AcceptOutT.push_back(pClient);
				pGTcpSvrOvertimeCount++;
			}
			pClient = pClient->pNext;
		}

		LeaveCriticalSection(&GTcpSvrPendingAcceptCS);

		while(pGTcpSvrOvertimeCount)
		{
			pGTcpSvrOvertimeCount--;
			GTcpSvr_DoCloseClient(PPER_HANDLE_DATA(AcceptOutT[pGTcpSvrOvertimeCount]->pOwner), AcceptOutT[pGTcpSvrOvertimeCount]);
			//LSCloseSocket_cs(AcceptOutT[pGTcpSvrOvertimeCount]);
		}
		AcceptOutT.clear();
#endif
	}

	return 0;
}