#include "ss_init_socket.h"

BOOL InitializeWinsock2()
{
	int nRet = 0;
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);
	nRet = WSAStartup(wVersionRequested, &wsaData);
	if (0 != nRet)
		return FALSE;

	if (2 != HIBYTE(wsaData.wVersion) || 2 != LOBYTE(wsaData.wVersion))
	{
		WSACleanup();
		return FALSE;
	}

	return TRUE;
}

BOOL InitializeListenSocket(PPER_HANDLE_DATA pPerHandleData, HANDLE hIocp, const char* g_Port)
{
	int nRet = 0;
	struct addrinfo hints;
	struct addrinfo *addrlocal = NULL;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_IP;

	if (0 != getaddrinfo(NULL, g_Port, &hints, &addrlocal))
		return FALSE;

	if (NULL == addrlocal)
		return FALSE;

	pPerHandleData->s_LinkSock = WSASocket(addrlocal->ai_family, 
		addrlocal->ai_socktype,
		addrlocal->ai_protocol, 
		NULL, 
		0,
		WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == pPerHandleData->s_LinkSock)
	{
		freeaddrinfo(addrlocal);
		return FALSE;
	}

	if (NULL == CreateIoCompletionPort((HANDLE)pPerHandleData->s_LinkSock, hIocp, (ULONG_PTR)pPerHandleData, 0))
	{
		freeaddrinfo(addrlocal);
		return FALSE;
	}

	nRet = bind(pPerHandleData->s_LinkSock, addrlocal->ai_addr, addrlocal->ai_addrlen);
	if (SOCKET_ERROR == nRet)
	{
		freeaddrinfo(addrlocal);
		return FALSE;
	}

	nRet = listen(pPerHandleData->s_LinkSock, SOMAXCONN);
	if (SOCKET_ERROR == nRet)
	{
		freeaddrinfo(addrlocal);
		return FALSE;
	}

	int nZero = 0;
	nRet = setsockopt(pPerHandleData->s_LinkSock, SOL_SOCKET, SO_SNDBUF, (char*)&nZero, sizeof(nZero));
	if (SOCKET_ERROR == nRet)
	{
		freeaddrinfo(addrlocal);
		return FALSE;
	}

	freeaddrinfo(addrlocal);
	return TRUE;
}

void LSCloseSocket(PPER_IO_DATA pPerIoData)
{
	if (INVALID_SOCKET == pPerIoData->s_AcceptSock)
		return;
	LINGER lingerStruct;
	lingerStruct.l_linger = 0;
	lingerStruct.l_onoff = 1;
	setsockopt(pPerIoData->s_AcceptSock, SOL_SOCKET, SO_LINGER,
		(char*)&lingerStruct, sizeof(lingerStruct));
	
	closesocket(pPerIoData->s_AcceptSock);
	pPerIoData->s_AcceptSock = INVALID_SOCKET;
}

void LSCloseSocket_cs(PPER_HANDLE_DATA pPerHandleData)
{
	if (NULL == pPerHandleData)
		return;
	EnterCriticalSection(&pPerHandleData->cs);
	if (INVALID_SOCKET == pPerHandleData->s_LinkSock)
	{
		LeaveCriticalSection(&pPerHandleData->cs);
		return;
	}
	LINGER lingerStruct;
	lingerStruct.l_linger = 0;
	lingerStruct.l_onoff = 1;
	setsockopt(pPerHandleData->s_LinkSock, SOL_SOCKET, SO_LINGER,
		(char*)&lingerStruct, sizeof(lingerStruct));

	CancelIo((HANDLE)pPerHandleData->s_LinkSock);

	closesocket(pPerHandleData->s_LinkSock);
	pPerHandleData->s_LinkSock = INVALID_SOCKET;
	LeaveCriticalSection(&pPerHandleData->cs);
}

void* GSock_WSAGetExtensionFunctionPointer(SOCKET Socket, GUID ExFuncGuid)
{
	DWORD dwOut;
	void *pResult;
	WSAIoctl(Socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &ExFuncGuid, sizeof(GUID), &pResult, sizeof(void *), &dwOut, NULL, NULL);

	return(pResult);
}

BOOL GetAcceptexAndGetAcceptexSockAddr()
{
	SOCKET Socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == Socket)
	{
		return FALSE;
	}
	GUID Guid1 = WSAID_ACCEPTEX;
	GUID Guid2 = WSAID_DISCONNECTEX;
	GUID Guid3 = WSAID_GETACCEPTEXSOCKADDRS;
	GUID Guid4 = WSAID_CONNECTEX;
	GUID Guid5 = WSAID_TRANSMITFILE;
	GUID Guid6 = WSAID_TRANSMITPACKETS;

	pfnSockAcceptEx = (LPFN_ACCEPTEX)GSock_WSAGetExtensionFunctionPointer(Socket, Guid1);
	pfnSockDisconnectEx = (LPFN_DISCONNECTEX)GSock_WSAGetExtensionFunctionPointer(Socket, Guid2);
	pfnSockGetAcceptExSockAddrs = (LPFN_GETACCEPTEXSOCKADDRS)GSock_WSAGetExtensionFunctionPointer(Socket, Guid3);
	pfnSockConnectEx = (LPFN_CONNECTEX)GSock_WSAGetExtensionFunctionPointer(Socket, Guid4);
	pfnSockTransmitFile = (LPFN_TRANSMITFILE)GSock_WSAGetExtensionFunctionPointer(Socket, Guid5);
	pfnSockTransmitPackets = (LPFN_TRANSMITPACKETS)GSock_WSAGetExtensionFunctionPointer(Socket, Guid6);

	closesocket(Socket);
	Socket = INVALID_SOCKET;
	return TRUE;
}