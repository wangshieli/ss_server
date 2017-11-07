#include "ss_do_process.h"
#include "ss_init_socket.h"
#include "ss_post_reques.h"
#include "tool.h"
#include "ss_memory.h"

BOOL ss_verify_main_link(PPER_HANDLE_DATA pPerHandle, PPER_IO_DATA pPerIoData, HANDLE hIocp)
{
	char* pHostInfoBegin = strstr(pPerIoData->buf, "host_id=");
	char* pHostInfoEnd = strstr(pPerIoData->buf, "IAMPROXYEND");
	if (NULL == pHostInfoBegin || NULL == pHostInfoEnd)
	{
		LSCloseSocket_cs(pPerHandle);
		FreeHandleData(pPerHandle);
		FreeIoData(pPerIoData);
		return FALSE;
	}

	int nTempLength = strlen("host_id=");
	int nHostIdLength = pHostInfoEnd - pHostInfoBegin - nTempLength;
	memcpy(pPerHandle->cHostId, pHostInfoBegin + nTempLength, nHostIdLength);
	
	EnterCriticalSection(&g_csCountOfMLink);
	if (++g_dwCountOfMLink > SS_MAX_MAIN_LINK)
	{
		// 想代理发送说明
		g_dwCountOfMLink--;
		ss_dispose_yesno_process1(pPerHandle, pPerIoData);
		LeaveCriticalSection(&g_csCountOfMLink);
		return FALSE;
	}
	myprintf("代理链接总数:%d", g_dwCountOfMLink);
	LeaveCriticalSection(&g_csCountOfMLink);

	PPER_HANDLE_DATA pListenDataForClient = AllocHandleData(DT_LISTEN_CLIENT);
	if (NULL == pListenDataForClient)
	{
		EnterCriticalSection(&g_csCountOfMLink);
		_InterlockedDecrement(&g_dwCountOfMLink);
		LeaveCriticalSection(&g_csCountOfMLink);
		LSCloseSocket_cs(pPerHandle);
		FreeHandleData(pPerHandle);
		FreeIoData(pPerIoData);
		return FALSE;
	}
	if (!InitializeListenSocket(pListenDataForClient, hIocp, "0"))
	{
		EnterCriticalSection(&g_csCountOfMLink);
		_InterlockedDecrement(&g_dwCountOfMLink);
		LeaveCriticalSection(&g_csCountOfMLink);
		LSCloseSocket_cs(pListenDataForClient);
		FreeHandleData(pListenDataForClient);

		LSCloseSocket_cs(pPerHandle);
		FreeHandleData(pPerHandle);
		FreeIoData(pPerIoData);

		return FALSE;
	}

	// 获取端口号
	sockaddr_in addr;
	int laddr = sizeof(sockaddr_in);
	if (0 != getsockname(pListenDataForClient->s_LinkSock, (sockaddr*)&addr, &laddr))
	{
		EnterCriticalSection(&g_csCountOfMLink);
		_InterlockedDecrement(&g_dwCountOfMLink);
		LeaveCriticalSection(&g_csCountOfMLink);
		LSCloseSocket_cs(pListenDataForClient);
		FreeHandleData(pListenDataForClient);

		LSCloseSocket_cs(pPerHandle);
		FreeHandleData(pPerHandle);
		FreeIoData(pPerIoData);

		return FALSE;
	}

	pPerHandle->pCPPair = pListenDataForClient;
	pListenDataForClient->pCPPair = pPerHandle;

	for (int i = 0; i < SS_MAX_CLIENT_ACCEPT; )
	{
		PPER_IO_DATA pAcceptClient = AllocIoData();
		if(NULL == pAcceptClient)
			continue;
		pAcceptClient->s_ListenSock = pListenDataForClient->s_LinkSock;
		pAcceptClient->pOwner = pListenDataForClient;
		_InterlockedIncrement(&pPerHandle->nRefCount);
		if (!PostAccept(pAcceptClient, OP_ACCEPT_CLIENT_LINK))
		{
			if (_InterlockedDecrement(&pPerHandle->nRefCount) < 0)
			{
				EnterCriticalSection(&g_csCountOfMLink);
				_InterlockedDecrement(&g_dwCountOfMLink);
				LeaveCriticalSection(&g_csCountOfMLink);
				FreeHandleData(pPerHandle->pCPPair);
				FreeHandleData(pPerHandle);
				FreeIoData(pPerIoData);
				FreeIoData(pAcceptClient);
				return FALSE;
			}
			FreeIoData(pAcceptClient);
		}
		i++;
	}

	pPerHandle->dwThePoll = GetTickCount();

	EnterCriticalSection(&g_csMainLinkMap);
	SS_MAP_TYPE::iterator it = g_MainLinkMap.find(pPerHandle->cHostId);
	if (it != g_MainLinkMap.end())
	{
		LSCloseSocket_cs(it->second->pCPPair);
		LSCloseSocket_cs(it->second);
		g_MainLinkMap.erase(it);
	}
	g_MainLinkMap.insert(SS_MAP_TYPE::value_type(pPerHandle->cHostId, pPerHandle));
	LeaveCriticalSection(&g_csMainLinkMap);

	// 发送端口信息到代理
	char *pInfo = "  IAMNEWPORTBEGINpost=%dIAMNEWPORTEND-Proxy5001IsRead";
	sprintf_s(pPerIoData->buf, pInfo, ntohs(addr.sin_port));
	int len = strlen(pPerIoData->buf+2);
	pPerIoData->buf[0] = len >> 8;
	pPerIoData->buf[1] = len;
	pPerIoData->nAlreadyRecvBytes = len+2;
	pPerIoData->nAlreadySendBytes = 0;
	if (!PostSend(pPerHandle, pPerIoData, OP_SEND_CLIENT_LISTEN_PORT_2_PROXY))
	{
		EnterCriticalSection(&g_csCountOfMLink);
		_InterlockedDecrement(&g_dwCountOfMLink);
		LeaveCriticalSection(&g_csCountOfMLink);
		LSCloseSocket_cs(pPerHandle->pCPPair);
		LSCloseSocket_cs(pPerHandle);
		if (_InterlockedDecrement(&pPerHandle->nRefCount) < 0)
		{
			EnterCriticalSection(&g_csMainLinkMap);
			SS_MAP_TYPE::iterator it = g_MainLinkMap.find(pPerHandle->cHostId);
			if (it != g_MainLinkMap.end())
				if (it->second == pPerHandle)
					g_MainLinkMap.erase(it);
			LeaveCriticalSection(&g_csMainLinkMap);
			FreeHandleData(pPerHandle->pCPPair);
			FreeHandleData(pPerHandle);
		}
		FreeIoData(pPerIoData);

		return FALSE;
	}

	return TRUE;
}

BOOL ss_dispose_client_link(PPER_HANDLE_DATA pPerHandle, PPER_IO_DATA pPerIoData, HANDLE hIocp)
{
	PPER_HANDLE_DATA pListenDataForProxy = AllocHandleData(DT_LISTEN_PROXY);
	if (NULL == pListenDataForProxy)
	{
		LSCloseSocket_cs(pPerHandle);
		FreeHandleData(pPerHandle);
		FreeIoData(pPerIoData);
		return FALSE;
	}
	if (!InitializeListenSocket(pListenDataForProxy, hIocp, "0"))
	{
		LSCloseSocket_cs(pListenDataForProxy);
		FreeHandleData(pListenDataForProxy);
		LSCloseSocket_cs(pPerHandle);
		FreeHandleData(pPerHandle);
		FreeIoData(pPerIoData);
		return FALSE;
	}

	// 获取端口号
	sockaddr_in addr;
	int laddr = sizeof(sockaddr_in);
	if (0 != getsockname(pListenDataForProxy->s_LinkSock, (sockaddr*)&addr, &laddr))
	{
		LSCloseSocket_cs(pListenDataForProxy);
		FreeHandleData(pListenDataForProxy);
		LSCloseSocket_cs(pPerHandle);
		FreeHandleData(pPerHandle);
		FreeIoData(pPerIoData);

		return FALSE;
	}

	pPerHandle->pCPPair = pListenDataForProxy;
	pListenDataForProxy->pCPPair = pPerHandle;

	getguid(pListenDataForProxy->cKey);
	pListenDataForProxy->dwThePoll = GetTickCount();
	EnterCriticalSection(&g_csListenMap);
	g_ListenMap.insert(SS_MAP_TYPE::value_type(pListenDataForProxy->cKey, pListenDataForProxy));
	LeaveCriticalSection(&g_csListenMap);

	char* pInfo = "  IAMNEWPORTBEGINpost=%dIAMNEWPORTEND";
	sprintf_s(pPerIoData->buf, pInfo, ntohs(addr.sin_port));
	int len = strlen(pPerIoData->buf+2);
	pPerIoData->buf[0] = len >> 8;
	pPerIoData->buf[1] = len;
	pPerIoData->nAlreadyRecvBytes = len+2;
	pPerIoData->nAlreadySendBytes = 0;
	pPerHandle->pHelper->pHelper = pPerHandle;
	if (!PostSend(pPerHandle->pHelper, pPerIoData, OP_SEND_PROXY_LISTEN_PORT_2_PROXY))
	{
		LSCloseSocket_cs(pListenDataForProxy);
		LSCloseSocket_cs(pPerHandle);
		EnterCriticalSection(&g_csListenMap);
		if (_InterlockedDecrement(&pListenDataForProxy->nRefCount) < 0)
		{
			SS_MAP_TYPE::iterator it = g_ListenMap.find(pListenDataForProxy->cKey);
			if (it != g_ListenMap.end())
			{		
				g_ListenMap.erase(it);
			}
			FreeHandleData(pListenDataForProxy);
			FreeHandleData(pPerHandle);
		}
		LeaveCriticalSection(&g_csListenMap);

		FreeIoData(pPerIoData);
		return FALSE;
	}

	PPER_IO_DATA pListenIoForProxy = AllocIoData();
	pListenIoForProxy->s_ListenSock = pListenDataForProxy->s_LinkSock;
	pListenIoForProxy->pOwner = pListenDataForProxy;
	if (!PostAccept(pListenIoForProxy, OP_ACCEPT_PROXY_LINK))
	{
		LSCloseSocket_cs(pListenDataForProxy);
		LSCloseSocket_cs(pPerHandle);
		EnterCriticalSection(&g_csListenMap);
		if (_InterlockedDecrement(&pListenDataForProxy->nRefCount) < 0)
		{
			SS_MAP_TYPE::iterator it = g_ListenMap.find(pListenDataForProxy->cKey);
			if (it != g_ListenMap.end())
			{		
				g_ListenMap.erase(it);
			}
			FreeHandleData(pListenDataForProxy);
			FreeHandleData(pPerHandle);
		}
		LeaveCriticalSection(&g_csListenMap);
		FreeIoData(pListenIoForProxy);

		return TRUE;
	}

	return TRUE;
}

BOOL ss_dispose_redail_process(PPER_HANDLE_DATA pPerHandle, PPER_IO_DATA pPerIoData)
{
	if (strncmp("IAMPROXY", pPerIoData->buf, 8) == 0)
	{
		char* pHostInfoBegin = strstr(pPerIoData->buf, "host_id=");
		char* pHostInfoEnd = strstr(pPerIoData->buf, "IAMPROXY6086END");
		if (NULL == pHostInfoBegin || NULL == pHostInfoEnd)
		{
			LSCloseSocket_cs(pPerHandle);
			FreeHandleData(pPerHandle);
			FreeIoData(pPerIoData);
			return FALSE;
		}

		int nTempLength = strlen("host_id=");
		int nHostIdLength = pHostInfoEnd - pHostInfoBegin - nTempLength;
		memcpy(pPerHandle->cHostId, pHostInfoBegin + nTempLength, nHostIdLength);
		myprintf("代理%s链接到server", pPerHandle->cHostId);
		EnterCriticalSection(&g_csRedailLinkMap);
		SS_MAP_TYPE::iterator it = g_RedailLinkMap.find(pPerHandle->cHostId);
		if (it != g_RedailLinkMap.end())
		{
			LSCloseSocket_cs(it->second);
			g_RedailLinkMap.erase(it);
		}
		g_RedailLinkMap.insert(SS_MAP_TYPE::value_type(pPerHandle->cHostId, pPerHandle));
		LeaveCriticalSection(&g_csRedailLinkMap);

		pPerIoData->nAlreadyRecvBytes = pPerIoData->nAlreadySendBytes = 0;
		if (!PostZeroByteRecv(pPerHandle, pPerIoData, OP_ZERO_RECV_FOR_REDAIL_EXIT))
		{
			LSCloseSocket_cs(pPerHandle);
			EnterCriticalSection(&g_csRedailLinkMap);
			if (_InterlockedDecrement(&pPerHandle->nRefCount) < 0)
			{
				SS_MAP_TYPE::iterator it = g_RedailLinkMap.find(pPerHandle->cHostId);
				if (it != g_RedailLinkMap.end())
					if (it->second == pPerHandle)
						g_RedailLinkMap.erase(it);
				FreeHandleData(pPerHandle);
			}
			LeaveCriticalSection(&g_csRedailLinkMap);
			FreeIoData(pPerIoData);
		}

	}else
	{
		char _HostId[32];
		ZeroMemory(_HostId, 32);
		char* pHostInfoBegin = strstr(pPerIoData->buf, "host_id=");
		if (NULL == pHostInfoBegin)
		{
			LSCloseSocket_cs(pPerHandle);
			FreeHandleData(pPerHandle);
			FreeIoData(pPerIoData);
			return FALSE;
		}
		char* pHostInfoEnd = strstr(pHostInfoBegin, "&");
		if (NULL == pHostInfoEnd)
		{
			pHostInfoEnd = strchr(pHostInfoBegin, '\0');
		}
		if (NULL == pHostInfoEnd)
		{
			LSCloseSocket_cs(pPerHandle);
			FreeHandleData(pPerHandle);
			FreeIoData( pPerIoData);
			return FALSE;
		}

		int nTempLength = strlen("host_id=");
		int nHostIdLength = pHostInfoEnd - pHostInfoBegin - nTempLength;
		memcpy(_HostId, pHostInfoBegin + nTempLength, nHostIdLength);

		PPER_HANDLE_DATA pRedailLinkData = NULL;
		EnterCriticalSection(&g_csRedailLinkMap);
		SS_MAP_TYPE::iterator it = g_RedailLinkMap.find(_HostId);
		if (it != g_RedailLinkMap.end())
		{
			_InterlockedIncrement(&it->second->nRefCount);
			pRedailLinkData = it->second;
		}
		LeaveCriticalSection(&g_csRedailLinkMap);

		if (NULL != pRedailLinkData)
		{
			DWORD dwCurrentTick = GetTickCount();
			if ((dwCurrentTick - pRedailLinkData->dwThePoll) < 30)
			{
				EnterCriticalSection(&g_csRedailLinkMap);
				if (_InterlockedDecrement(&pRedailLinkData->nRefCount) < 0)
				{
					SS_MAP_TYPE::iterator it = g_RedailLinkMap.find(pRedailLinkData->cHostId);
					if (it != g_RedailLinkMap.end())
						if (it->second == pRedailLinkData)
							g_RedailLinkMap.erase(it);
					FreeHandleData(pRedailLinkData);
				}
				LeaveCriticalSection(&g_csRedailLinkMap);
				
				ss_dispose_ok_process(pPerHandle, pPerIoData);
				return TRUE;
			}
			pRedailLinkData->dwThePoll = GetTickCount();
			pPerHandle->pHelper = pRedailLinkData;
			myprintf("向代理%s发送的6086请求", _HostId);
			if (!PostSend(pRedailLinkData, pPerIoData, OP_SEND_REDAIL_INFO_2_PROXY))
			{
				LSCloseSocket_cs(pRedailLinkData);
				EnterCriticalSection(&g_csRedailLinkMap);
				if (_InterlockedDecrement(&pRedailLinkData->nRefCount) < 0)
				{
					SS_MAP_TYPE::iterator it = g_RedailLinkMap.find(pRedailLinkData->cHostId);
					if (it != g_RedailLinkMap.end())
						if (it->second == pRedailLinkData)
							g_RedailLinkMap.erase(it);
					FreeHandleData(pRedailLinkData);
				}
				LeaveCriticalSection(&g_csRedailLinkMap);

				LSCloseSocket_cs(pPerHandle);
				FreeHandleData(pPerHandle);
				FreeIoData(pPerIoData);
			}
		}else
		{
			myprintf("代理%s没有链接", _HostId);
			ss_dispose_ok_process(pPerHandle, pPerIoData);
		}
	}

	return TRUE;
}

BOOL ss_dispose_ok_process(PPER_HANDLE_DATA pPerHandle, PPER_IO_DATA pPerIoData)
{
	if (strncmp("/chmode", pPerIoData->buf + 5, 7) == 0)
	{
		int len = strlen("HTTP/1.1 200 OK\r\n");
		memcpy(pPerIoData->buf, "HTTP/1.1 200 OK\r\n", len);
		pPerIoData->nAlreadyRecvBytes = len;
		pPerIoData->nAlreadySendBytes = 0;
	}else
	{
		memcpy(pPerIoData->buf, "ok", 2);
		pPerIoData->nAlreadyRecvBytes = 2;
		pPerIoData->nAlreadySendBytes = 0;
	}
	if (!PostSend(pPerHandle, pPerIoData, OP_SEND_OK_INFO_2_CLIENT))
	{
		LSCloseSocket_cs(pPerHandle);
		FreeHandleData(pPerHandle);
		FreeIoData(pPerIoData);
		return FALSE;
	}

	return TRUE;
}

BOOL ss_dispose_yesno_process1(PPER_HANDLE_DATA pPerHandle, PPER_IO_DATA pPerIoData)
{
	char *pInfo = "  IAMNEWPORTBEGINpost=%sIAMNEWPORTEND-Proxy5001IsRead";
	sprintf_s(pPerIoData->buf, pInfo, "0000");
	int len = strlen(pPerIoData->buf+2);
	pPerIoData->buf[0] = len >> 8;
	pPerIoData->buf[1] = len;
	pPerIoData->nAlreadyRecvBytes = len+2;
	pPerIoData->nAlreadySendBytes = 0;
	if (!PostSend(pPerHandle, pPerIoData, OP_ZERO_RECV_FOR_MAIN_LINK_EXIT_EX))
	{
		LSCloseSocket_cs(pPerHandle);
		FreeHandleData(pPerHandle);
		FreeIoData(pPerIoData);

		return FALSE;
	}

	return TRUE;
}

BOOL ss_dispose_yesno_process(PPER_HANDLE_DATA pPerHandle, PPER_IO_DATA pPerIoData, BOOL bYes)
{
	if (bYes)
	{
		memcpy(pPerIoData->buf, "YES", 3);
		pPerIoData->nAlreadyRecvBytes = 3;
	}
	else
	{
		memcpy(pPerIoData->buf, "NO", 2);
		pPerIoData->nAlreadyRecvBytes = 2;
	}
	
	if (!PostSend(pPerHandle, pPerIoData, OP_SEND_YESNO_2_MAIN_LINK))
	{
		LSCloseSocket_cs(pPerHandle);
		FreeHandleData(pPerHandle);
		FreeIoData(pPerIoData);
		return FALSE;
	}

	return TRUE;
}

BOOL ss_dispose_hb_process(PPER_HANDLE_DATA pPerHandle, PPER_IO_DATA pPerIoData)
{
	char* pHostInfoBegin = strstr(pPerIoData->buf, "host_id=");
	char* pHostInfoEnd = strstr(pPerIoData->buf, "IAMPROXYEND");
	if (NULL == pHostInfoBegin || NULL == pHostInfoEnd)
	{
		LSCloseSocket_cs(pPerHandle);
		FreeHandleData(pPerHandle);
		FreeIoData(pPerIoData);
		return FALSE;
	}

	int nTempLength = strlen("host_id=");
	int nHostIdLength = pHostInfoEnd - pHostInfoBegin - nTempLength;
	memcpy(pPerHandle->cHostId, pHostInfoBegin + nTempLength, nHostIdLength);

	EnterCriticalSection(&g_csHBMap);
	pPerHandle->dwThePoll = GetTickCount();
	SS_MAP_TYPE::iterator it = g_HBMap.find(pPerHandle->cHostId);
	if (it != g_HBMap.end())
	{
		LSCloseSocket_cs(it->second);
		g_HBMap.erase(it);
	}
	g_HBMap.insert(SS_MAP_TYPE::value_type(pPerHandle->cHostId, pPerHandle));
	LeaveCriticalSection(&g_csHBMap);

	pPerIoData->not = OP_RECV_FROM_HB_LINK2;
	pPerIoData->nAlreadyRecvBytes = 0;
	if (!PostZeroByteRecv(pPerHandle, pPerIoData, OP_ZERO_RECV))
	{
		LSCloseSocket_cs(pPerHandle);
		EnterCriticalSection(&g_csHBMap);
		if (_InterlockedDecrement(&pPerHandle->nRefCount) < 0)
		{
			SS_MAP_TYPE::iterator it = g_HBMap.find(pPerHandle->cHostId);
			if (it != g_HBMap.end())
			{
				if (it->second == pPerHandle)
					g_HBMap.erase(it);
			}
			FreeHandleData(pPerHandle);
		}
		LeaveCriticalSection(&g_csHBMap);
		FreeIoData(pPerIoData);
	}

	return TRUE;
}

BOOL ss_dispose_hb_process2(PPER_HANDLE_DATA pPerHandle, PPER_IO_DATA pPerIoData)
{
	pPerHandle->dwThePoll = GetTickCount();

	pPerIoData->not = OP_RECV_FROM_HB_LINK2;
	pPerIoData->nAlreadyRecvBytes = 0;
	if (!PostZeroByteRecv(pPerHandle, pPerIoData, OP_ZERO_RECV))
	{
		LSCloseSocket_cs(pPerHandle);
		EnterCriticalSection(&g_csHBMap);
		if (_InterlockedDecrement(&pPerHandle->nRefCount) < 0)
		{
			SS_MAP_TYPE::iterator it = g_HBMap.find(pPerHandle->cHostId);
			if (it != g_HBMap.end())
			{
				if (it->second == pPerHandle)
					g_HBMap.erase(it);
			}
			FreeHandleData(pPerHandle);
		}
		LeaveCriticalSection(&g_csHBMap);
		FreeIoData(pPerIoData);
	}

	return TRUE;
}