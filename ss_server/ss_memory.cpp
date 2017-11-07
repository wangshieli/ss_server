#include "ss_memory.h"

PPER_HANDLE_DATA AllocHandleData(device_type _dt)
{
	PPER_HANDLE_DATA pAlloc = NULL;
	EnterCriticalSection(&g_csPerHandleDq);
	if (!g_PerHandleDq.empty())
	{
		pAlloc = g_PerHandleDq.front();
		g_PerHandleDq.pop_front();
		LeaveCriticalSection(&g_csPerHandleDq);
		pAlloc->clean(_dt);
		return pAlloc;
	}else
	{
		LeaveCriticalSection(&g_csPerHandleDq);
		pAlloc = (PPER_HANDLE_DATA)xmalloc(SS_SIZE_OF_PER_HANDLE_DATA);
		if (pAlloc)
		{
			InitializeCriticalSection(&pAlloc->cs);
			pAlloc->clean(_dt);
		}

		return pAlloc;
	}
}

void FreeHandleData(PPER_HANDLE_DATA pFree)
{
	if (pFree == NULL)
		return;
	EnterCriticalSection(&g_csPerHandleDq);
	if (g_PerHandleDq.size() < SS_MAX_SIZE_HANDLE_DEQUE)
	{
		g_PerHandleDq.push_back(pFree);
		LeaveCriticalSection(&g_csPerHandleDq);
	}else
	{
		LeaveCriticalSection(&g_csPerHandleDq);
		DeleteCriticalSection(&pFree->cs);
		xfree(pFree);
		pFree = NULL;
	}
}

PPER_IO_DATA AllocIoData()
{
	PPER_IO_DATA pAlloc = NULL;
	EnterCriticalSection(&g_csPerIoDq);
	if (!g_PerIoDq.empty())
	{
		pAlloc = g_PerIoDq.front();
		g_PerIoDq.pop_front();
		LeaveCriticalSection(&g_csPerIoDq);
		pAlloc->clean();
		return pAlloc;
	}else
	{
		LeaveCriticalSection(&g_csPerIoDq);
		pAlloc = (PPER_IO_DATA)xmalloc(SS_SIZE_OF_PER_IO_DATA);
		if (pAlloc)
		{
			pAlloc->clean();
		}
		return pAlloc;
	}
}

void FreeIoData(PPER_IO_DATA pFree)
{
	if (pFree == NULL)
		return;
	EnterCriticalSection(&g_csPerIoDq);
	if (g_PerIoDq.size() < SS_MAX_SIZE_IO_DEQUE)
	{
		g_PerIoDq.push_back(pFree);
		LeaveCriticalSection(&g_csPerIoDq);
	}else
	{
		LeaveCriticalSection(&g_csPerIoDq);
		xfree(pFree);
		pFree = NULL;
	}
}