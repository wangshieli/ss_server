#include "ss_global_resource.h"
#include "ss_init_socket.h"
#include "ss_worker.h"
#include "ss_post_reques.h"
#include "ss_timer.h"
#include "ss_memory.h"
#include "ss_log_show.h"
#include "ss_accept_function.h"
#include "ss_accept_thread.h"

int main()
{
	g_MainLinkMap.clear();
	InitializeCriticalSection(&g_csMainLinkMap);
	g_RedailLinkMap.clear();
	InitializeCriticalSection(&g_csRedailLinkMap);
	g_HBMap.clear();
	InitializeCriticalSection(&g_csHBMap);
	g_ListenMap.clear();
	InitializeCriticalSection(&g_csListenMap);
	g_ProxyMap.clear();
	InitializeCriticalSection(&g_csProxyMap);
	g_PerHandleDq.clear();
	g_PerIoDq.clear();
	InitializeCriticalSection(&g_csPerHandleDq);
	InitializeCriticalSection(&g_csPerIoDq);
	InitializeCriticalSection(&g_csCountOfMLink);
	InitializeCriticalSection(&GTcpSvrPendingAcceptCS);

	if (!InitializeWinsock2())
	{
		cout << "1" << endl;
		return -1;
	}

	hWorkerIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	hAcceptIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if (NULL == hWorkerIocp || NULL == hAcceptIocp)
	{
		cout << "2" << endl;
		WSACleanup();
		return -1;
	}

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	DWORD dwNumOfCpu = si.dwNumberOfProcessors;

	// Æô¶¯worker
	for (DWORD i = 0; i < dwNumOfCpu * 2; )
	{
		HANDLE hWorkerThread = (HANDLE)_beginthreadex(NULL, 0, ss_worker_thread, hWorkerIocp, 0, NULL);
		if (NULL == hWorkerThread)
			continue;

		CloseHandle(hWorkerThread);
		hWorkerThread = NULL;
		i++;
	}

	for (DWORD i = 0; i < dwNumOfCpu; i++)
	{
		HANDLE hWorkerThread = (HANDLE)_beginthreadex(NULL, 0, ss_accept_thread, NULL, 0, NULL);
		if (NULL == hWorkerThread)
			continue;

		CloseHandle(hWorkerThread);
		hWorkerThread = NULL;
		i++;
	}

	HANDLE hLog = (HANDLE)_beginthreadex(NULL, 0, ss_log_show, NULL, 0, &g_uiLogThreadId);

	// ³õÊ¼»¯¼àÌýsocket
	PPER_HANDLE_DATA pMainLinkListener = AllocHandleData(DT_MAIN_LINK_LISTENER);
	if (!InitializeListenSocket(pMainLinkListener, hAcceptIocp, SS_MAIN_LISTEN_SOCKET))
	{
		cout << "3" << endl;
		if (INVALID_SOCKET != pMainLinkListener->s_LinkSock)
		{
			closesocket(pMainLinkListener->s_LinkSock);
			pMainLinkListener->s_LinkSock = INVALID_SOCKET;
		}
		CloseHandle(hWorkerIocp);
		WSACleanup();
		return -1;
	}
	pMainLinkListener->pfnOnAccept = &ss_accept;
	pMainLinkListener->pfnOnAcceptErr = &ss_accepterr;

	PPER_HANDLE_DATA pRedailLinkListener = AllocHandleData(DT_REDAIL_LINK_LISTENER);
	if (!InitializeListenSocket(pRedailLinkListener, hAcceptIocp, SS_REDAIL_LISTEN_SOCKET))
	{
		if (INVALID_SOCKET != pRedailLinkListener->s_LinkSock)
		{
			closesocket(pRedailLinkListener->s_LinkSock);
			pRedailLinkListener->s_LinkSock = INVALID_SOCKET;
		}

		CloseHandle(hWorkerIocp);
		WSACleanup();
		return -1;
	}
	pRedailLinkListener->pfnOnAccept = &ss_accept;
	pRedailLinkListener->pfnOnAcceptErr = &ss_accepterr;

	PPER_HANDLE_DATA pHBLinkListener = AllocHandleData(DT_HB_LINK_LISTENER);
	if (!InitializeListenSocket(pHBLinkListener, hAcceptIocp, SS_HB_LISTEN_SOCKET))
	{
		if (INVALID_SOCKET != pHBLinkListener->s_LinkSock)
		{
			closesocket(pHBLinkListener->s_LinkSock);
			pHBLinkListener->s_LinkSock = INVALID_SOCKET;
		}

		CloseHandle(hWorkerIocp);
		WSACleanup();
		return -1;
	}
	pHBLinkListener->pfnOnAccept = &ss_accept;
	pHBLinkListener->pfnOnAcceptErr = &ss_accepterr;

	if (!GetAcceptexAndGetAcceptexSockAddr())
	{
		cout << "4" << endl;
		CloseHandle(hWorkerIocp);
		WSACleanup();
		return -1;
	}

	for (int i = 0; i < SS_MAX_MAIN_ACCEPT; i++)
	{
		if (!PostAcceptEx(pMainLinkListener, DT_MAIN_LINK))
			return -1;
	}
	g_hSvrListener[g_nSvrListenerCount] = pMainLinkListener;
	g_hSvrListenerEvent[g_nSvrListenerCount] = CreateEvent(NULL, FALSE, FALSE, NULL);
	WSAEventSelect(pMainLinkListener->s_LinkSock, g_hSvrListenerEvent[g_nSvrListenerCount], FD_ACCEPT);
	g_nSvrListenerCount++;

	for (int i = 0; i < SS_MAX_MAIN_ACCEPT; i++)
	{
		if (!PostAcceptEx(pRedailLinkListener, DT_REDAIL_LINK))
			return -1;
	}
	g_hSvrListener[g_nSvrListenerCount] = pRedailLinkListener;
	g_hSvrListenerEvent[g_nSvrListenerCount] = CreateEvent(NULL, FALSE, FALSE, NULL);
	WSAEventSelect(pRedailLinkListener->s_LinkSock, g_hSvrListenerEvent[g_nSvrListenerCount], FD_ACCEPT);
	g_nSvrListenerCount++;

	for (int i = 0; i < SS_MAX_MAIN_ACCEPT; i++)
	{
		if (!PostAcceptEx(pHBLinkListener, DT_HB_LINK))
			return -1;
	}
	g_hSvrListener[g_nSvrListenerCount] = pHBLinkListener;
	g_hSvrListenerEvent[g_nSvrListenerCount] = CreateEvent(NULL, FALSE, FALSE, NULL);
	WSAEventSelect(pHBLinkListener->s_LinkSock, g_hSvrListenerEvent[g_nSvrListenerCount], FD_ACCEPT);
	g_nSvrListenerCount++;

	_beginthreadex(NULL, 0, ss_servo_threadMainLink, NULL, 0, NULL);
	_beginthreadex(NULL, 0, ss_servo_threadRedailLink, NULL, 0, NULL);
	_beginthreadex(NULL, 0, ss_servo_threadHBLink, NULL, 0, NULL);

	HANDLE hTimerThreadId = (HANDLE)_beginthreadex(NULL, 0, ss_timer, NULL, 0, NULL);
	if (hTimerThreadId == NULL)
		return -1;

	CloseHandle(hTimerThreadId);

	getchar();
	return 0;
}