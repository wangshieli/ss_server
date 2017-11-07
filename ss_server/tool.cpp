#include "tool.h"
#include "ss_init_socket.h"
#include "ss_memory.h"

void myprintf(const char* lpFormat, ...)
{
	char *pLog = new char[256];
	ZeroMemory(pLog, 256);

	va_list arglist;
	va_start(arglist, lpFormat);
	vsprintf_s(pLog, 256, lpFormat, arglist);
	va_end(arglist);
	PostThreadMessage(g_uiLogThreadId, SS_LOG_SHOW, (WPARAM)pLog, NULL);
}

void SS_TEXT(const char* lpFormat, ...)
{
	char *pLog = new char[256];
	ZeroMemory(pLog, 256);

	va_list arglist;
	va_start(arglist, lpFormat);
	vsprintf_s(pLog, 256, lpFormat, arglist);
	va_end(arglist);
	PostThreadMessage(g_uiLogThreadId, SS_LOG_TEXT, (WPARAM)pLog, NULL);
}

void ErrorCode2Text(DWORD dwErr)
{
	switch(dwErr)
	{
	case WSAEFAULT:
		cout << "WSAEFAULT	The buf parameter is not completely contained in a valid part of the user address space." << endl;;
		break; 
	case WSAENOTCONN:
		cout << "WSAENOTCONN	The socket is not connected." << endl;; 
		break;
	case WSAEINTR:
		cout << "WSAEINTR	The (blocking) call was canceled through WSACancelBlockingCall.	" << endl;; 
		break;
	case WSAENOTSOCK:
		cout << " WSAENOTSOCK	The descriptor s is not a socket." << endl;; 
		break; 
	case WSANOTINITIALISED:
		cout << "WSANOTINITIALISED: A successful WSAStartup call must occur before using this function." << endl;;
		break; 
	case WSAENETDOWN:
		cout << "WSAENETDOWN	The network subsystem has failed." << endl;; 
		break;
	case WSAEINPROGRESS:
		cout << "WSAEINPROGRESS	A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function." << endl;; 
		break;
	case WSAENETRESET:
		cout << " WSAENETRESET	The connection has been broken due to the keep-alive activity detecting a failure while the operation was in progress." << endl;; 
		break; 
	case WSAEOPNOTSUPP:
		cout << "WSAEOPNOTSUPP	MSG_OOB was specified, but the socket is not stream-style such as type SOCK_STREAM, OOB data is not supported in the communication domain associated with this socket, or the socket is unidirectional and supports only send operations.	" << endl; 
		break; 
	case WSAESHUTDOWN:
		cout << "WSAESHUTDOWN	The socket has been shut down; it is not possible to receive on a socket after shutdown has been invoked with how set to SD_RECEIVE or SD_BOTH." << endl;; 
		break;
	case WSAEWOULDBLOCK:
		cout << " WSAEWOULDBLOCK	The socket is marked as nonblocking and the receive operation would block.	" << endl;; 
		break; 
	case WSAEMSGSIZE:
		cout << " WSAENOTSOCK		The message was too large to fit into the specified buffer and was truncated." << endl;; 
		break;
	case WSAEINVAL:
		cout << "WSAEINVAL	The socket has not been bound with bind, or an unknown flag was specified, or MSG_OOB was specified for a socket with SO_OOBINLINE enabled or (for byte stream sockets only) len was zero or negative.	" << endl; 
	case WSAECONNABORTED:
		cout << " 	WSAECONNABORTED	The virtual circuit was terminated due to a time-out or other failure. The application should close the socket as it is no longer usable." << endl;
		break; 
	case WSAETIMEDOUT:
		cout << "WSAETIMEDOUT	The connection has been dropped because of a network failure or because the peer system failed to respond.	" << endl;; 
		break; 
	case WSAECONNRESET:
		//error="WSAECONNRESET	The virtual circuit was reset by the remote side executing a hard or abortive close."; 
		cout << "WSAECONNRESET Connection dropped.." << endl;;
		break;

	default:
		break;
	}
}

void ss_check_hb_timeout()
{
	EnterCriticalSection(&g_csHBMap);
	SS_MAP_TYPE g_HBMapCopy(g_HBMap);
	SS_MAP_TYPE::iterator itr = g_HBMap.begin();
	for (; itr != g_HBMap.end(); itr++)
		_InterlockedIncrement(&itr->second->nRefCount);
	LeaveCriticalSection(&g_csHBMap);

	DWORD dwCurrentTick = GetTickCount();
	SS_MAP_TYPE::iterator it = g_HBMapCopy.begin();
	for (;it != g_HBMapCopy.end(); it++)
	{
		if (dwCurrentTick - it->second->dwThePoll > 15 * 1000)
		{
			LSCloseSocket_cs(it->second);
			EnterCriticalSection(&g_csHBMap);
			SS_MAP_TYPE::iterator it01 = g_HBMap.find(it->second->cHostId);
			if (it01 != g_HBMap.end())
			{
				if (it01->second == it->second)
					g_HBMap.erase(it01);
			}
			LeaveCriticalSection(&g_csHBMap);
			myprintf("代理%s心跳超时", it->second->cHostId);
			PPER_IO_DATA pTempIo = AllocIoData();
			pTempIo->pOwner = it->second;
			pTempIo->ot = OP_RELEASE_TIMEOUT_HB;
			PostQueuedCompletionStatus(hWorkerIocp, 0, (ULONG_PTR)it->second, &pTempIo->ol);
		}else
		{
			if (_InterlockedDecrement(&it->second->nRefCount) < 0)
			{
				EnterCriticalSection(&g_csHBMap);
				SS_MAP_TYPE::iterator it02 = g_HBMap.find(it->second->cHostId);
				if (it02 != g_HBMap.end())
					if (it02->second == it->second)
						g_HBMap.erase(it02);
				LeaveCriticalSection(&g_csHBMap);
				FreeHandleData(it->second);
			}
		}
	}

	g_HBMapCopy.clear();
}

void _stdcall ss_hb_timer(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime)
{
	ss_check_hb_timeout();
}

void _stdcall ss_proxy_timer(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime)
{
	ss_check_proxy_timeout();
}

void _stdcall ss_listen_timer(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime)
{
	ss_check_listen_timeout();
}

void ss_check_listen_timeout()
{
	EnterCriticalSection(&g_csListenMap);
	SS_MAP_TYPE g_ListenMapCopy(g_ListenMap);
	SS_MAP_TYPE::iterator itr = g_ListenMap.begin();
	for (; itr != g_ListenMap.end(); itr++)
		_InterlockedIncrement(&itr->second->nRefCount);
	LeaveCriticalSection(&g_csListenMap);

	DWORD dwCurrentTick = GetTickCount();
	SS_MAP_TYPE::iterator it = g_ListenMapCopy.begin();
	for (;it != g_ListenMapCopy.end(); it++)
	{
		if (dwCurrentTick - it->second->dwThePoll > 30 * 1000)
		{
			PPER_IO_DATA pTempIo = AllocIoData();
			pTempIo->pOwner = it->second;
			pTempIo->ot = OP_RELEASE_TIMEOUT_LISTEN;
			PostQueuedCompletionStatus(hWorkerIocp, 0, (ULONG_PTR)it->second, &pTempIo->ol);
		}else
		{
			if (_InterlockedDecrement(&it->second->nRefCount) < 0)
			{
				EnterCriticalSection(&g_csListenMap);
				SS_MAP_TYPE::iterator itr1 = g_ListenMap.find(it->second->cKey);
				if (itr1 != g_ListenMap.end())
					g_ListenMap.erase(itr1);
				LeaveCriticalSection(&g_csListenMap);
				FreeHandleData(it->second->pCPPair);
				FreeHandleData(it->second);
			}
		}
	}

	g_ListenMapCopy.clear();
}

void ss_check_proxy_timeout()
{
	EnterCriticalSection(&g_csProxyMap);
	SS_MAP_TYPE g_ProxyMapCopy(g_ProxyMap);
	SS_MAP_TYPE::iterator itr = g_ProxyMap.begin();
	for (; itr != g_ProxyMap.end(); itr++)
		_InterlockedIncrement(&itr->second->nRefCount);
	LeaveCriticalSection(&g_csProxyMap);

	DWORD dwCurrentTick = GetTickCount();
	SS_MAP_TYPE::iterator it = g_ProxyMapCopy.begin();
	for (;it != g_ProxyMapCopy.end(); it++)
	{
		if (dwCurrentTick - it->second->dwThePoll > 10 * 1000)
		{
			PPER_IO_DATA pTempIo = AllocIoData();
			pTempIo->pOwner = it->second;
			pTempIo->ot = OP_RELEASE_TIMEOUT_PROXY;
			PostQueuedCompletionStatus(hWorkerIocp, 0, (ULONG_PTR)it->second, &pTempIo->ol);
		}else
		{
			if (_InterlockedDecrement(&it->second->nRefCount) < 0)
			{
				EnterCriticalSection(&g_csProxyMap);
				SS_MAP_TYPE::iterator itr1 = g_ProxyMap.find(it->second->cKey);
				if (itr1 != g_ProxyMap.end())
					g_ProxyMap.erase(itr1);
				LeaveCriticalSection(&g_csProxyMap);
				FreeHandleData(it->second->pCPPair);
				FreeHandleData(it->second);
			}
		}
	}

	g_ProxyMapCopy.clear();
}

void getguid(char *buf)
{
	GUID guid;
	CoCreateGuid(&guid);
	sprintf_s(buf, 128, "%08X%04X%04x%02X%02X%02X%02X%02X%02X%02X%02X"
		,guid.Data1
		, guid.Data2
		, guid.Data3
		, guid.Data4[0], guid.Data4[1]   
		, guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5]
		, guid.Data4[6], guid.Data4[7]
		);
}

DWORD dwGTcpSvrPendingAcceptCount = 0;

void ss_insertpendingacceptlist(PPER_HANDLE_DATA pClient)
{
	EnterCriticalSection(&GTcpSvrPendingAcceptCS);

	pClient->pNext = pGTcpSvrPendingAcceptHead;
	if(pGTcpSvrPendingAcceptHead)
		pGTcpSvrPendingAcceptHead->pPrior = pClient;
	pClient->pPrior = NULL;
	pGTcpSvrPendingAcceptHead = pClient;
	dwGTcpSvrPendingAcceptCount++;

	LeaveCriticalSection(&GTcpSvrPendingAcceptCS);
}

void ss_deletependingacceptlist(PPER_HANDLE_DATA pClient)
{
	EnterCriticalSection(&GTcpSvrPendingAcceptCS);

	if(pClient == pGTcpSvrPendingAcceptHead)
	{
		pGTcpSvrPendingAcceptHead = pGTcpSvrPendingAcceptHead->pNext;
		if(pGTcpSvrPendingAcceptHead)
			pGTcpSvrPendingAcceptHead->pPrior = NULL;
	}else
	{
		pClient->pPrior->pNext = pClient->pNext;
		if(pClient->pNext)
			pClient->pNext->pPrior = pClient->pPrior;
	}
	dwGTcpSvrPendingAcceptCount--;

	LeaveCriticalSection(&GTcpSvrPendingAcceptCS);
}