#include "ss_post_reques.h"
#include "ss_memory.h"
#include "ss_init_socket.h"
#include "tool.h"

BOOL PostAccept(PPER_IO_DATA pAcceptIo, OP_TYPE ot)
{
	if (INVALID_SOCKET == pAcceptIo->s_ListenSock)
		return FALSE;

	DWORD dwBytes = 0;
	pAcceptIo->s_AcceptSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == pAcceptIo->s_AcceptSock)
		return FALSE;

	ZeroMemory(&pAcceptIo->ol, sizeof(pAcceptIo->ol));
	pAcceptIo->ot = ot;
	if (FALSE == pfnSockAcceptEx(pAcceptIo->s_ListenSock, pAcceptIo->s_AcceptSock, pAcceptIo->wsaBuf.buf, 0, 
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &dwBytes, &pAcceptIo->ol))
	{
		if (WSA_IO_PENDING != WSAGetLastError())
		{
			closesocket(pAcceptIo->s_AcceptSock);
			pAcceptIo->s_AcceptSock = INVALID_SOCKET;
			return FALSE;
		}
	}

	return TRUE;
}

BOOL PostAcceptEx(PPER_HANDLE_DATA pListener, device_type dt)
{
	if (INVALID_SOCKET == pListener->s_LinkSock)
		return FALSE;

	DWORD dwBytes = 0;
	PPER_HANDLE_DATA pClient = AllocHandleData(dt);
	if (NULL == pClient)
		return FALSE;
	pClient->s_LinkSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == pClient->s_LinkSock)
	{
		FreeHandleData(pClient);
		return FALSE;
	}

	PPER_IO_DATA pAcceptIo = AllocIoData();
	if (NULL == pAcceptIo)
	{
		LSCloseSocket_cs(pClient);
		FreeHandleData(pClient);
		FreeIoData(pAcceptIo);
		return FALSE;
	}

	if (NULL == CreateIoCompletionPort((HANDLE)pClient->s_LinkSock,
		hWorkerIocp, (DWORD)pClient, 0))
	{
		LSCloseSocket_cs(pClient);
		FreeHandleData(pClient);
		FreeIoData(pAcceptIo);
		return FALSE;
	}

	pAcceptIo->pOwner = pClient;
	pClient->pAcceptIo = pAcceptIo;
	pClient->pOwner = pListener;
	pAcceptIo->wsaBuf.len = SS_MAX_DATA;
#ifdef USE_DOS_SAVE
	ss_insertpendingacceptlist(pClient);
#endif
	if (FALSE == pfnSockAcceptEx(pListener->s_LinkSock, pClient->s_LinkSock, 
		pAcceptIo->wsaBuf.buf, 
		pAcceptIo->wsaBuf.len - ((sizeof(SOCKADDR_IN)+16)*2), 
		sizeof(SOCKADDR_IN) + 16, 
		sizeof(SOCKADDR_IN) + 16, 
		&dwBytes, &pAcceptIo->ol))
	{
		if (WSA_IO_PENDING != WSAGetLastError())
		{
#ifdef USE_DOS_SAVE
			ss_deletependingacceptlist(pClient);
#endif
			LSCloseSocket_cs(pClient);
			FreeHandleData(pClient);
			FreeIoData(pAcceptIo);
			return FALSE;
		}
	}

	return TRUE;
}

BOOL PostZeroByteRecv(PPER_HANDLE_DATA pPerHandleData, PPER_IO_DATA pPerIoData, OP_TYPE ot)
{
	if (INVALID_SOCKET == pPerHandleData->s_LinkSock)
		return FALSE;

	DWORD dwFlags = MSG_PARTIAL;
	DWORD dwTrans = 0;
	pPerIoData->wsaBuf.buf = NULL;
	pPerIoData->wsaBuf.len = 0;
	pPerIoData->ot = ot;
	ZeroMemory(&pPerIoData->ol, sizeof(pPerIoData->ol));
	int nRet = WSARecv(pPerHandleData->s_LinkSock, &pPerIoData->wsaBuf, 1, &dwTrans, &dwFlags, &pPerIoData->ol, NULL);
	if (SOCKET_ERROR == nRet && WSA_IO_PENDING != WSAGetLastError())
	{
		return FALSE;
	}

	return TRUE;
}

BOOL PostRecv(PPER_HANDLE_DATA pPerHandleData, PPER_IO_DATA pPerIoData, OP_TYPE ot)
{
	if (INVALID_SOCKET == pPerHandleData->s_LinkSock)
		return FALSE;

	DWORD dwFlags = MSG_PARTIAL;
	DWORD dwTrans = 0;
	pPerIoData->wsaBuf.buf = pPerIoData->buf + pPerIoData->nAlreadyRecvBytes;
	pPerIoData->wsaBuf.len = SS_MAX_DATA - pPerIoData->nAlreadyRecvBytes;
	pPerIoData->ot = ot;
	ZeroMemory(&pPerIoData->ol, sizeof(pPerIoData->ol));
	int nRet = WSARecv(pPerHandleData->s_LinkSock, &pPerIoData->wsaBuf, 1, &dwTrans, &dwFlags, &pPerIoData->ol, NULL);
	if (SOCKET_ERROR == nRet && WSA_IO_PENDING != WSAGetLastError())
	{
		return FALSE;
	}

	return TRUE;	
}

BOOL PostZeroByteSend(PPER_HANDLE_DATA pPerHandleData, PPER_IO_DATA pPerIoData, OP_TYPE ot)
{
	if (INVALID_SOCKET == pPerHandleData->s_LinkSock)
		return FALSE;

	DWORD dwFlags = MSG_PARTIAL;
	DWORD dwTrans = 0;
	pPerIoData->wsaBuf.buf = NULL;
	pPerIoData->wsaBuf.len = 0;
	pPerIoData->ot = ot;
	ZeroMemory(&pPerIoData->ol, sizeof(pPerIoData->ol));
	int nRet = WSASend(pPerHandleData->s_LinkSock, &pPerIoData->wsaBuf, 1, 
		&dwTrans, dwFlags, &pPerIoData->ol, NULL);
	if (SOCKET_ERROR == nRet && WSA_IO_PENDING != WSAGetLastError())
	{
		return FALSE;
	}

	return TRUE;
}

BOOL PostSend(PPER_HANDLE_DATA pPerHandleData, PPER_IO_DATA pPerIoData, OP_TYPE ot)
{
	if (INVALID_SOCKET == pPerHandleData->s_LinkSock)
		return FALSE;

	DWORD dwTrans = 0;
	DWORD dwFlags = 0;
	ZeroMemory(&pPerIoData->ol, sizeof(pPerIoData->ol));
	pPerIoData->wsaBuf.buf = pPerIoData->buf + pPerIoData->nAlreadySendBytes;
	pPerIoData->wsaBuf.len = pPerIoData->nAlreadyRecvBytes - pPerIoData->nAlreadySendBytes;
	pPerIoData->ot = ot;
	int nRet = WSASend(pPerHandleData->s_LinkSock, &pPerIoData->wsaBuf, 1, 
		&dwTrans, dwFlags, &pPerIoData->ol, NULL);
	if (SOCKET_ERROR == nRet && WSA_IO_PENDING != WSAGetLastError())
	{
		return FALSE;
	}

	return TRUE;
}