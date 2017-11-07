#include "ss_accept_function.h"
#include "tool.h"
#include "ss_do_process.h"
#include "ss_post_reques.h"
#include "ss_init_socket.h"
#include "ss_memory.h"

void ss_accept(DWORD dwBytes, PPER_HANDLE_DATA pListener, PPER_IO_DATA pPerIoData)
{
	if(!dwBytes)
	{
		ss_accepterr(pListener, pPerIoData);
		return;
	}

	PPER_HANDLE_DATA pClient = PPER_HANDLE_DATA(pPerIoData->pOwner);
#ifdef USE_DOS_SAVE
	ss_deletependingacceptlist(pClient);
#endif

	pPerIoData->ot = OP_DO_ALL_ACCEPT;
	BOOL bSuccess = PostQueuedCompletionStatus(hWorkerIocp, 0, (DWORD)pClient, &pPerIoData->ol);

	if ((!bSuccess && WSA_IO_PENDING != WSAGetLastError()))
	{            
		LSCloseSocket_cs(pClient);
		FreeHandleData(pClient);
		FreeIoData(pPerIoData);
	}
}

void ss_accepterr(PPER_HANDLE_DATA pListener, PPER_IO_DATA pIoData)
{
	PPER_HANDLE_DATA pClient = PPER_HANDLE_DATA(pIoData->pOwner);
#ifdef USE_DOS_SAVE
	ss_deletependingacceptlist(pClient);
#endif

	LSCloseSocket_cs(pClient);
	FreeHandleData(pClient);
	FreeIoData(pIoData);
}