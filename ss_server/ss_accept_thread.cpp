#include "ss_accept_thread.h"

unsigned int _stdcall ss_accept_thread(LPVOID pVoid)
{
	BOOL bResult;
	DWORD dwBytes;
	PGIOCP_COMPLETION_KEY pCompletionKey;
	LPOVERLAPPED pOverlapped;

	for(;;)
	{
		bResult = GetQueuedCompletionStatus(hAcceptIocp, &dwBytes, (PULONG_PTR)&pCompletionKey, &pOverlapped, INFINITE);		

		if(!pOverlapped)
			continue;		

		if(bResult)
		{
			pCompletionKey->pfnOnIocpOper(dwBytes, pCompletionKey, pOverlapped);
		}else
		{
			pCompletionKey->pfnOnIocpError(pCompletionKey, pOverlapped);
		}
	}
	return 0;
}
