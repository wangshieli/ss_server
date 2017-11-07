#include "ss_worker.h"
#include "ss_post_reques.h"
#include "ss_init_socket.h"
#include "ss_do_process.h"
#include "tool.h"
#include "ss_memory.h"

unsigned int _stdcall ss_worker_thread(LPVOID pVoid)
{
	HANDLE hIocp = (HANDLE)pVoid;

	PPER_HANDLE_DATA pPerHandleData = NULL;
	PPER_IO_DATA pPerIoData = NULL;
	WSAOVERLAPPED* lpOverlapped = NULL;
	DWORD dwTrans = 0;
	DWORD dwFlags = 0;

	while (true)
	{
		BOOL bRet = GetQueuedCompletionStatus(hIocp, &dwTrans, (PULONG_PTR)&pPerHandleData, 
			(LPOVERLAPPED*)&lpOverlapped, INFINITE);

		if (!bRet)// 调用CancelIo时，返回false
		{
			//	DWORD dwIOError = WSAGetLastError();
			//	ErrorCode2Text(dwIOError);
			if (NULL != pPerHandleData)
			{
				pPerIoData = CONTAINING_RECORD(lpOverlapped, PER_IO_DATA, ol);
				if (NULL == pPerIoData)
					continue;

				switch (pPerIoData->ot)
				{
				case OP_DO_ALL_ACCEPT:
					{
						PPER_HANDLE_DATA pClient = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pClient);
						FreeHandleData(pClient);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_ACCEPT_CLIENT_LINK:
					{
						SS_TEXT("OP_ACCEPT_CLIENT_LINK：ErrCode = %d, HndType = %d, IoType = %d",
							WSAGetLastError(), pPerHandleData->dt, pPerIoData->ot);
						PPER_HANDLE_DATA pListener = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pListener->pCPPair);
						LSCloseSocket_cs(pListener);
						if (_InterlockedDecrement(&pListener->pCPPair->nRefCount) < 0)
						{
							EnterCriticalSection(&g_csMainLinkMap);
							SS_MAP_TYPE::iterator it = g_MainLinkMap.find(pListener->pCPPair->cHostId);
							if (it != g_MainLinkMap.end())
								if (it->second == pListener->pCPPair)
									g_MainLinkMap.erase(it);
							LeaveCriticalSection(&g_csMainLinkMap);
							FreeHandleData(pListener->pCPPair);
							FreeHandleData(pListener);
						}
						LSCloseSocket(pPerIoData);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_ZERO_RECV:
					{
						switch (pPerIoData->not)
						{
						case OP_RECV_FROM_MAIN_LINK:
							{
								PPER_HANDLE_DATA pMainLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
								LSCloseSocket_cs(pMainLink);
								FreeHandleData(pMainLink);
								FreeIoData(pPerIoData);
							}
							break;

						case OP_RECV_FORM_CLIENT_LINK:
							{
								LSCloseSocket_cs(pPerHandleData);
								LSCloseSocket_cs(pPerHandleData->pCPPair);
								EnterCriticalSection(&g_csProxyMap);
								if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
								{
									SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
									if (it != g_ProxyMap.end())
										g_ProxyMap.erase(it);
									FreeHandleData(pPerHandleData->pCPPair);
									FreeHandleData(pPerHandleData);
								}
								LeaveCriticalSection(&g_csProxyMap);
								FreeIoData(pPerIoData);
							}
							break;

						case OP_RECV_FROM_PROXY_LINK:
							{
								LSCloseSocket_cs(pPerHandleData);
								LSCloseSocket_cs(pPerHandleData->pCPPair);
								EnterCriticalSection(&g_csProxyMap);
								if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
								{
									SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->cKey);
									if (it != g_ProxyMap.end())
										g_ProxyMap.erase(it);
									FreeHandleData(pPerHandleData->pCPPair);
									FreeHandleData(pPerHandleData);
								}
								LeaveCriticalSection(&g_csProxyMap);
								FreeIoData(pPerIoData);
							}
							break;

						case OP_RECV_FROM_CLIENT_CONNECT:
							{
								LSCloseSocket_cs(pPerHandleData);
								LSCloseSocket_cs(pPerHandleData->pCPPair);
								EnterCriticalSection(&g_csProxyMap);
								if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
								{
									SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
									if (it != g_ProxyMap.end())
										g_ProxyMap.erase(it);
									FreeHandleData(pPerHandleData->pCPPair);
									FreeHandleData(pPerHandleData);
								}
								LeaveCriticalSection(&g_csProxyMap);

								FreeIoData(pPerIoData);
							}
							break;

						case OP_RECV_FROM_PROXY_CONNECT:
							{
								LSCloseSocket_cs(pPerHandleData);
								LSCloseSocket_cs(pPerHandleData->pCPPair);
								EnterCriticalSection(&g_csProxyMap);
								if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
								{
									SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->cKey);
									if (it != g_ProxyMap.end())
										g_ProxyMap.erase(it);
									FreeHandleData(pPerHandleData->pCPPair);
									FreeHandleData(pPerHandleData);
								}
								LeaveCriticalSection(&g_csProxyMap);

								FreeIoData(pPerIoData);
							}
							break;

						case OP_RECV_FROM_REDAIL_LINK:
							{
								PPER_HANDLE_DATA pRedailLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
								LSCloseSocket_cs(pRedailLink);
								FreeHandleData(pRedailLink);
								FreeIoData(pPerIoData);
							}
							break;

						case OP_RECV_FROM_HB_LINK:
							{
								PPER_HANDLE_DATA pHBLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
								LSCloseSocket_cs(pHBLink);
								FreeHandleData(pHBLink);
								FreeIoData(pPerIoData);
							}
							break;

						case OP_RECV_FROM_HB_LINK2:
							{
								PPER_HANDLE_DATA pHBLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
								LSCloseSocket_cs(pHBLink);
								EnterCriticalSection(&g_csHBMap);
								if (_InterlockedDecrement(&pHBLink->nRefCount) < 0)
								{
									SS_MAP_TYPE::iterator it = g_HBMap.find(pHBLink->cHostId);
									if (it != g_HBMap.end())
									{
										if (it->second == pHBLink)
											g_HBMap.erase(it);
									}
									FreeHandleData(pHBLink);
								}
								LeaveCriticalSection(&g_csHBMap);
								FreeIoData(pPerIoData);
							}
							break;
						default:
							break;
						}
					}
					break;

				case OP_RECV_FROM_MAIN_LINK:
					{
						PPER_HANDLE_DATA pMainLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pMainLink);
						FreeHandleData(pMainLink);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_SEND_CLIENT_LISTEN_PORT_2_PROXY:
					{
						EnterCriticalSection(&g_csCountOfMLink);
						_InterlockedDecrement(&g_dwCountOfMLink);
						LeaveCriticalSection(&g_csCountOfMLink);
						PPER_HANDLE_DATA pMainLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pMainLink->pCPPair);
						LSCloseSocket_cs(pMainLink);
						if (_InterlockedDecrement(&pMainLink->nRefCount) < 0)
						{
							EnterCriticalSection(&g_csMainLinkMap);
							SS_MAP_TYPE::iterator it = g_MainLinkMap.find(pMainLink->cHostId);
							if (it != g_MainLinkMap.end())
								if (it->second == pMainLink)
									g_MainLinkMap.erase(it);
							LeaveCriticalSection(&g_csMainLinkMap);
							FreeHandleData(pMainLink->pCPPair);
							FreeHandleData(pMainLink);
						}
						FreeIoData(pPerIoData);
					}
					break;

				case OP_ZERO_RECV_FOR_MAIN_LINK_EXIT:
					{
						PPER_HANDLE_DATA pMainLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						EnterCriticalSection(&g_csCountOfMLink);
						_InterlockedDecrement(&g_dwCountOfMLink);
						LeaveCriticalSection(&g_csCountOfMLink);
						LSCloseSocket_cs(pMainLink->pCPPair);
						LSCloseSocket_cs(pMainLink);
						if (_InterlockedDecrement(&pMainLink->nRefCount) < 0)
						{
							EnterCriticalSection(&g_csMainLinkMap);
							SS_MAP_TYPE::iterator it = g_MainLinkMap.find(pMainLink->cHostId);
							if (it != g_MainLinkMap.end())
								if (it->second == pMainLink)
									g_MainLinkMap.erase(it);
							LeaveCriticalSection(&g_csMainLinkMap);
							FreeHandleData(pMainLink->pCPPair);
							FreeHandleData(pMainLink);
						}
						FreeIoData(pPerIoData);
					}
					break;

				case OP_ZERO_RECV_FOR_MAIN_LINK_EXIT_EX:
					{
						PPER_HANDLE_DATA pMainLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pMainLink);
						FreeHandleData(pMainLink);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_ZERO_RECV_FOR_REDAIL_EXIT:
					{
						PPER_HANDLE_DATA pRedailLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pRedailLink);
						EnterCriticalSection(&g_csRedailLinkMap);
						if (_InterlockedDecrement(&pRedailLink->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_RedailLinkMap.find(pRedailLink->cHostId);
							if (it != g_RedailLinkMap.end())
								if (it->second == pRedailLink)
									g_RedailLinkMap.erase(it);
							FreeHandleData(pRedailLink);
						}
						LeaveCriticalSection(&g_csRedailLinkMap);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_RECV_FORM_CLIENT_LINK:
					{
						LSCloseSocket_cs(pPerHandleData);
						LSCloseSocket_cs(pPerHandleData->pCPPair);
						EnterCriticalSection(&g_csProxyMap);
						if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
							if (it != g_ProxyMap.end())
								g_ProxyMap.erase(it);
							FreeHandleData(pPerHandleData->pCPPair);
							FreeHandleData(pPerHandleData);
						}
						LeaveCriticalSection(&g_csProxyMap);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_ACCEPT_PROXY_LINK:
					{
						PPER_HANDLE_DATA pLinstener = PPER_HANDLE_DATA(pPerIoData->pOwner);
						SS_TEXT("OP_ACCEPT_PROXY_LINK：ErrCode = %d", 
							WSAGetLastError());
						LSCloseSocket_cs(pLinstener->pCPPair);
						LSCloseSocket_cs(pLinstener);
						EnterCriticalSection(&g_csListenMap);
						if (_InterlockedDecrement(&pLinstener->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ListenMap.find(pLinstener->cKey);
							if (it != g_ListenMap.end())
							{		
								g_ListenMap.erase(it);
							}
							FreeHandleData(pLinstener->pCPPair);
							FreeHandleData(pLinstener);
						}
						LeaveCriticalSection(&g_csListenMap);

						LSCloseSocket(pPerIoData);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_SEND_PROXY_LISTEN_PORT_2_PROXY:
					{
						PPER_HANDLE_DATA pMainLink = PPER_HANDLE_DATA(pPerIoData->pOwner)->pHelper;
						LSCloseSocket_cs(pMainLink->pCPPair);
						LSCloseSocket_cs(pMainLink);
						//LSCloseSocket_cs(pPerHandleData->pHelper);
						//LSCloseSocket_cs(pPerHandleData->pHelper->pCPPair);
						FreeIoData(pPerIoData);

						if (_InterlockedDecrement(&pMainLink->nRefCount) < 0)
						{
							EnterCriticalSection(&g_csMainLinkMap);
							SS_MAP_TYPE::iterator it = g_MainLinkMap.find(pMainLink->cHostId);
							if (it != g_MainLinkMap.end())
								if (it->second == pMainLink)
									g_MainLinkMap.erase(it);
							LeaveCriticalSection(&g_csMainLinkMap);
							FreeHandleData(pMainLink->pCPPair);
							FreeHandleData(pMainLink);// 释放mianlink,需要引用同步
						}
					}
					break;

				case OP_SEDN_CLIENT_INFO_2_PROXY:
					{
						LSCloseSocket_cs(pPerHandleData);
						LSCloseSocket_cs(pPerHandleData->pCPPair);
						EnterCriticalSection(&g_csProxyMap);
						if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->cKey);
							if (it != g_ProxyMap.end())
								g_ProxyMap.erase(it);
							FreeHandleData(pPerHandleData->pCPPair);
							FreeHandleData(pPerHandleData);
						}
						LeaveCriticalSection(&g_csProxyMap);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_RECV_FROM_PROXY_LINK:
					{
						LSCloseSocket_cs(pPerHandleData);
						LSCloseSocket_cs(pPerHandleData->pCPPair);
						EnterCriticalSection(&g_csProxyMap);
						if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->cKey);
							if (it != g_ProxyMap.end())
								g_ProxyMap.erase(it);
							FreeHandleData(pPerHandleData->pCPPair);
							FreeHandleData(pPerHandleData);
						}
						LeaveCriticalSection(&g_csProxyMap);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_SEND_PROXY_INFO_2_CLIENT:
					{
						LSCloseSocket_cs(pPerHandleData);
						LSCloseSocket_cs(pPerHandleData->pCPPair);
						EnterCriticalSection(&g_csProxyMap);
						if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
							if (it != g_ProxyMap.end())
								g_ProxyMap.erase(it);
							FreeHandleData(pPerHandleData->pCPPair);
							FreeHandleData(pPerHandleData);
						}
						LeaveCriticalSection(&g_csProxyMap);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_RECV_FROM_CLIENT_CONNECT:
					{
						LSCloseSocket_cs(pPerHandleData);
						LSCloseSocket_cs(pPerHandleData->pCPPair);
						EnterCriticalSection(&g_csProxyMap);
						if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
							if (it != g_ProxyMap.end())
								g_ProxyMap.erase(it);
							FreeHandleData(pPerHandleData->pCPPair);
							FreeHandleData(pPerHandleData);
						}
						LeaveCriticalSection(&g_csProxyMap);

						FreeIoData(pPerIoData);
					}
					break;

				case OP_RECV_FROM_PROXY_CONNECT:
					{
						LSCloseSocket_cs(pPerHandleData);
						LSCloseSocket_cs(pPerHandleData->pCPPair);
						EnterCriticalSection(&g_csProxyMap);
						if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->cKey);
							if (it != g_ProxyMap.end())
								g_ProxyMap.erase(it);
							FreeHandleData(pPerHandleData->pCPPair);
							FreeHandleData(pPerHandleData);
						}
						LeaveCriticalSection(&g_csProxyMap);

						FreeIoData(pPerIoData);
					}
					break;

				case OP_SEND_CLIENT_INFO_2_PROXY_CONNECT:
					{
						LSCloseSocket_cs(pPerHandleData);
						LSCloseSocket_cs(pPerHandleData->pCPPair);
						EnterCriticalSection(&g_csProxyMap);
						if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->cKey);
							if (it != g_ProxyMap.end())
								g_ProxyMap.erase(it);
							FreeHandleData(pPerHandleData->pCPPair);
							FreeHandleData(pPerHandleData);
						}
						LeaveCriticalSection(&g_csProxyMap);

						FreeIoData(pPerIoData);
					}
					break;

				case OP_SEND_PROXY_INFO_2_CLIENT_CONNECT:
					{
						LSCloseSocket_cs(pPerHandleData);
						LSCloseSocket_cs(pPerHandleData->pCPPair);
						EnterCriticalSection(&g_csProxyMap);
						if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
							if (it != g_ProxyMap.end())
								g_ProxyMap.erase(it);
							FreeHandleData(pPerHandleData->pCPPair);
							FreeHandleData(pPerHandleData);
						}
						LeaveCriticalSection(&g_csProxyMap);

						FreeIoData(pPerIoData);
					}
					break;

				case OP_RECV_FROM_REDAIL_LINK:
					{
						PPER_HANDLE_DATA pRedailLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pRedailLink);
						FreeHandleData(pRedailLink);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_SEND_REDAIL_INFO_2_PROXY:
					{
						PPER_HANDLE_DATA pRedailLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pRedailLink->pHelper);
						EnterCriticalSection(&g_csRedailLinkMap);
						if (_InterlockedDecrement(&pRedailLink->pHelper->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_RedailLinkMap.find(pRedailLink->pHelper->cHostId);
							if (it != g_RedailLinkMap.end())
								if (it->second == pRedailLink->pHelper)
									g_RedailLinkMap.erase(it);
							FreeHandleData(pRedailLink->pHelper);
						}
						LeaveCriticalSection(&g_csRedailLinkMap);
						LSCloseSocket_cs(pRedailLink);
						FreeHandleData(pRedailLink);
						//	FreeHandleData(pPerHandleData);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_SEND_OK_INFO_2_CLIENT:
					{
						LSCloseSocket_cs(pPerHandleData);
						FreeHandleData(pPerHandleData);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_SEND_YESNO_2_MAIN_LINK:
					{
						PPER_HANDLE_DATA pMainLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pMainLink);
						FreeHandleData(pMainLink);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_RECV_FROM_HB_LINK:
					{
						PPER_HANDLE_DATA pHBLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pHBLink);
						FreeHandleData(pHBLink);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_RECV_FROM_HB_LINK2:
					{
						PPER_HANDLE_DATA pHBLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pHBLink);
						EnterCriticalSection(&g_csHBMap);
						if (_InterlockedDecrement(&pHBLink->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_HBMap.find(pHBLink->cHostId);
							if (it != g_HBMap.end())
							{
								if (it->second == pHBLink)
									g_HBMap.erase(it);
							}
							FreeHandleData(pHBLink);
						}
						LeaveCriticalSection(&g_csHBMap);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_RELEASE_TIMEOUT_HB:
					{
						PPER_HANDLE_DATA pHBOut = PPER_HANDLE_DATA(pPerIoData->pOwner);
						FreeIoData(pPerIoData);
						EnterCriticalSection(&g_csMainLinkMap);
						SS_MAP_TYPE::iterator it = g_MainLinkMap.find(pHBOut->cHostId);
						if (it != g_MainLinkMap.end())
						{
							if ((GetTickCount() - it->second->dwThePoll) < 20*1000)
							{
								LeaveCriticalSection(&g_csMainLinkMap);
								if (_InterlockedDecrement(&pHBOut->nRefCount) < 0)
									FreeHandleData(pHBOut);
								break;
							}
							LSCloseSocket_cs(it->second->pCPPair);
							LSCloseSocket_cs(it->second);
							g_MainLinkMap.erase(it);
						}
						LeaveCriticalSection(&g_csMainLinkMap);
						EnterCriticalSection(&g_csRedailLinkMap);
						SS_MAP_TYPE::iterator itr = g_RedailLinkMap.find(pHBOut->cHostId);
						if (itr != g_RedailLinkMap.end())
						{
							LSCloseSocket_cs(itr->second);
						}
						LeaveCriticalSection(&g_csRedailLinkMap);
						if (_InterlockedDecrement(&pHBOut->nRefCount) < 0)
							FreeHandleData(pHBOut);
					}
					break;

				case OP_RELEASE_TIMEOUT_PROXY:
					{
						PPER_HANDLE_DATA pProxyOut = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pProxyOut);
						LSCloseSocket_cs(pProxyOut->pCPPair);
						EnterCriticalSection(&g_csProxyMap);
						if (_InterlockedDecrement(&pProxyOut->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ProxyMap.find(pProxyOut->cKey);
							if (it != g_ProxyMap.end())
								g_ProxyMap.erase(it);
							FreeHandleData(pProxyOut->pCPPair);
							FreeHandleData(pProxyOut);
						}
						LeaveCriticalSection(&g_csProxyMap);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_RELEASE_TIMEOUT_LISTEN:
					{
						PPER_HANDLE_DATA pListenerOut = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pListenerOut->pCPPair);
						LSCloseSocket_cs(pListenerOut);
						EnterCriticalSection(&g_csListenMap);
						if (_InterlockedDecrement(&pListenerOut->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ListenMap.find(pListenerOut->cKey);
							if (it != g_ListenMap.end())
							{
								g_ListenMap.erase(it);
							}
							FreeHandleData(pListenerOut->pCPPair);
							FreeHandleData(pListenerOut);
						}	
						LeaveCriticalSection(&g_csListenMap);
						FreeIoData(pPerIoData);
					}
					break;
				default:
					break;
				}
			}
			//	continue;
		}else
		{
			pPerIoData = CONTAINING_RECORD(lpOverlapped, PER_IO_DATA, ol);
			if (NULL == pPerIoData)
				continue;

			if (0 == dwTrans)
			{
				switch (pPerIoData->ot)
				{
				case OP_DO_ALL_ACCEPT:
					{
						PPER_HANDLE_DATA pClient = PPER_HANDLE_DATA(pPerIoData->pOwner);
						SOCKADDR* pLocalSockaddr = NULL;
						SOCKADDR* pRemoteSockaddr = NULL;
						int LocalSockaddrLen = 0;
						int RemoteSockaddrLen = 0;
						pfnSockGetAcceptExSockAddrs(pPerIoData->wsaBuf.buf, 
							pPerIoData->wsaBuf.len - ((sizeof(SOCKADDR_IN)+16)*2), 
							sizeof(SOCKADDR_IN) + 16, 
							sizeof(SOCKADDR_IN) + 16, 
							&pLocalSockaddr, &LocalSockaddrLen, 
							&pRemoteSockaddr, &RemoteSockaddrLen);
						//myprintf("%s", pPerIoData->buf);

						unsigned long ulWaitBytes = 0;
						if (SOCKET_ERROR != ioctlsocket(pClient->s_LinkSock, FIONREAD, &ulWaitBytes))
						{
							if (ulWaitBytes > 0)
							{
								switch (pClient->dt)
								{
								case DT_MAIN_LINK:
									{
										pPerIoData->not = OP_RECV_FROM_MAIN_LINK;
									}
									break;
								case DT_REDAIL_LINK:
									{
										pPerIoData->not = OP_RECV_FROM_REDAIL_LINK;
									}
									break;
								case DT_HB_LINK:
									{
										pPerIoData->not = OP_RECV_FROM_HB_LINK;
									}
									break;
								default:
									{
										LSCloseSocket_cs(pClient);
										FreeHandleData(pClient);
										FreeIoData(pPerIoData);
									}
									break;
								}
								pPerIoData->nAlreadyRecvBytes = strlen(pPerIoData->buf);
								if (!PostZeroByteRecv(pClient, pPerIoData, OP_ZERO_RECV))
								{
									LSCloseSocket_cs(pClient);
									FreeHandleData(pClient);
									FreeIoData(pPerIoData);
								}
							}else
							{
								switch (pClient->dt)
								{
								case DT_MAIN_LINK:
									{
										ss_verify_main_link(pClient, pPerIoData, hWorkerIocp);
									}
									break;
								case DT_REDAIL_LINK:
									{
										pPerIoData->nAlreadyRecvBytes = strlen(pPerIoData->buf);
										ss_dispose_redail_process(pClient, pPerIoData);
									}
									break;
								case DT_HB_LINK:
									{
										ss_dispose_hb_process(pClient, pPerIoData);
									}
									break;
								default:
									{
										LSCloseSocket_cs(pClient);
										FreeHandleData(pClient);
										FreeIoData(pPerIoData);
									}
									break;
								}
							}
						}else
						{
							LSCloseSocket_cs(pClient);
							FreeHandleData(pClient);
							FreeIoData(pPerIoData);
						}
					}
					break;

				case OP_ACCEPT_MAIN_LINK:
					{
						PPER_HANDLE_DATA pNewMainLink = AllocHandleData(DT_MAIN_LINK);
						PPER_IO_DATA pNewMainLinkIo = AllocIoData();
						if (NULL == pNewMainLinkIo || NULL == pNewMainLink)
						{
							LSCloseSocket(pPerIoData);
							if (!PostAccept(pPerIoData, OP_ACCEPT_MAIN_LINK))
							{
								SS_TEXT("!!!OP_ACCEPT_MAIN_LINK：投递失败, ErrCode = %d", WSAGetLastError());
								FreeIoData(pPerIoData);
							}
							FreeHandleData(pNewMainLink);
							FreeIoData(pNewMainLinkIo);
							break;
						}
						pNewMainLink->s_LinkSock = pPerIoData->s_AcceptSock;
						pNewMainLinkIo->pOwner = pNewMainLink;

						// 重新投递mainlink接收
						if (!PostAccept(pPerIoData, OP_ACCEPT_MAIN_LINK))
						{
							SS_TEXT("!!!OP_ACCEPT_MAIN_LINK：投递失败, ErrCode = %d", WSAGetLastError());
							FreeIoData(pPerIoData);
						}

						if (NULL == CreateIoCompletionPort((HANDLE)pNewMainLink->s_LinkSock, hIocp, (ULONG_PTR)pNewMainLink, 0))
						{
							// 绑定完成端口失败
							LSCloseSocket_cs(pNewMainLink);
							FreeHandleData(pNewMainLink);
							FreeIoData(pNewMainLinkIo);
							break;
						}

						EnterCriticalSection(&g_csCountOfMLink);
						if (g_dwCountOfMLink > SS_MAX_MAIN_LINK)
						{
							// 想代理发送说明
							ss_dispose_yesno_process(pNewMainLink, pNewMainLinkIo, FALSE);
							LeaveCriticalSection(&g_csCountOfMLink);
							break;
						}
						ss_dispose_yesno_process(pNewMainLink, pNewMainLinkIo);
						LeaveCriticalSection(&g_csCountOfMLink);

						//// 投递接收mainlink发送来的信息
						//pNewMainLinkIo->not = OP_RECV_FROM_MAIN_LINK;
						//if (!PostZeroByteRecv(pNewMainLink, pNewMainLinkIo, OP_ZERO_RECV))
						//{
						//	LSCloseSocket_cs(pNewMainLink);
						//	FreeHandleData(pNewMainLink);
						//	FreeIoData(pNewMainLinkIo);
						//}
					}
					break;

				case OP_ZERO_RECV:
					{
						switch (pPerIoData->not)
						{
						case OP_RECV_FROM_MAIN_LINK:
							{
								PPER_HANDLE_DATA pMainLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
								if (!PostRecv(pMainLink, pPerIoData, OP_RECV_FROM_MAIN_LINK))
								{
									LSCloseSocket_cs(pMainLink);
									FreeHandleData(pMainLink);
									FreeIoData(pPerIoData);
								}
							}
							break;

						case OP_RECV_FORM_CLIENT_LINK:
							{
								if (!PostRecv(pPerHandleData, pPerIoData, OP_RECV_FORM_CLIENT_LINK))
								{
									LSCloseSocket_cs(pPerHandleData);
									LSCloseSocket_cs(pPerHandleData->pCPPair);
									EnterCriticalSection(&g_csProxyMap);
									if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
									{
										SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
										if (it != g_ProxyMap.end())
											g_ProxyMap.erase(it);
										FreeHandleData(pPerHandleData->pCPPair);
										FreeHandleData(pPerHandleData);
									}
									LeaveCriticalSection(&g_csProxyMap);
									FreeIoData(pPerIoData);
								}
							}
							break;

						case OP_RECV_FROM_PROXY_LINK:
							{
								if (!PostRecv(pPerHandleData, pPerIoData, OP_RECV_FROM_PROXY_LINK))
								{
									LSCloseSocket_cs(pPerHandleData);
									LSCloseSocket_cs(pPerHandleData->pCPPair);
									EnterCriticalSection(&g_csProxyMap);
									if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
									{
										SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->cKey);
										if (it != g_ProxyMap.end())
											g_ProxyMap.erase(it);
										FreeHandleData(pPerHandleData->pCPPair);
										FreeHandleData(pPerHandleData);
									}
									LeaveCriticalSection(&g_csProxyMap);
									FreeIoData(pPerIoData);
								}
							}
							break;

						case OP_RECV_FROM_CLIENT_CONNECT:
							{
								if (!PostRecv(pPerHandleData, pPerIoData, OP_RECV_FROM_CLIENT_CONNECT))
								{
									LSCloseSocket_cs(pPerHandleData);
									LSCloseSocket_cs(pPerHandleData->pCPPair);
									EnterCriticalSection(&g_csProxyMap);
									if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
									{
										SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
										if (it != g_ProxyMap.end())
											g_ProxyMap.erase(it);
										FreeHandleData(pPerHandleData->pCPPair);
										FreeHandleData(pPerHandleData);
									}
									LeaveCriticalSection(&g_csProxyMap);

									FreeIoData(pPerIoData);
								}
							}
							break;

						case OP_RECV_FROM_PROXY_CONNECT:
							{
								if (!PostRecv(pPerHandleData, pPerIoData, OP_RECV_FROM_PROXY_CONNECT))
								{
									LSCloseSocket_cs(pPerHandleData);
									LSCloseSocket_cs(pPerHandleData->pCPPair);
									EnterCriticalSection(&g_csProxyMap);
									if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
									{
										SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->cKey);
										if (it != g_ProxyMap.end())
											g_ProxyMap.erase(it);
										FreeHandleData(pPerHandleData->pCPPair);
										FreeHandleData(pPerHandleData);
									}
									LeaveCriticalSection(&g_csProxyMap);

									FreeIoData(pPerIoData);
								}
							}
							break;

						case OP_RECV_FROM_REDAIL_LINK:
							{
								PPER_HANDLE_DATA pRedailLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
								if (!PostRecv(pRedailLink, pPerIoData, OP_RECV_FROM_REDAIL_LINK))
								{
									LSCloseSocket_cs(pRedailLink);
									FreeHandleData(pRedailLink);
									FreeIoData(pPerIoData);
								}
							}
							break;

						case OP_RECV_FROM_HB_LINK:
							{
								PPER_HANDLE_DATA pHBLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
								if (!PostRecv(pHBLink, pPerIoData, OP_RECV_FROM_HB_LINK))
								{
									LSCloseSocket_cs(pHBLink);
									FreeHandleData(pHBLink);
									FreeIoData(pPerIoData);
								}
							}
							break;

						case OP_RECV_FROM_HB_LINK2:
							{
								PPER_HANDLE_DATA pHBLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
								if (!PostRecv(pHBLink, pPerIoData, OP_RECV_FROM_HB_LINK2))
								{
									LSCloseSocket_cs(pHBLink);
									EnterCriticalSection(&g_csHBMap);
									if (_InterlockedDecrement(&pHBLink->nRefCount) < 0)
									{
										SS_MAP_TYPE::iterator it = g_HBMap.find(pHBLink->cHostId);
										if (it != g_HBMap.end())
										{
											if (it->second == pHBLink)
												g_HBMap.erase(it);
										}
										FreeHandleData(pHBLink);
									}
									LeaveCriticalSection(&g_csHBMap);
									FreeIoData(pPerIoData);
								}
							}
							break;
						default:
							break;
						}
					}
					break;

				case OP_RECV_FROM_MAIN_LINK:
					{
						PPER_HANDLE_DATA pMainLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pMainLink);
						FreeHandleData(pMainLink);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_SEND_CLIENT_LISTEN_PORT_2_PROXY:
					{
						EnterCriticalSection(&g_csCountOfMLink);
						_InterlockedDecrement(&g_dwCountOfMLink);
						LeaveCriticalSection(&g_csCountOfMLink);
						PPER_HANDLE_DATA pMainLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pMainLink->pCPPair);
						LSCloseSocket_cs(pMainLink);
						if (_InterlockedDecrement(&pMainLink->nRefCount) < 0)
						{
							EnterCriticalSection(&g_csMainLinkMap);
							SS_MAP_TYPE::iterator it = g_MainLinkMap.find(pMainLink->cHostId);
							if (it != g_MainLinkMap.end())
								if (it->second == pMainLink)
									g_MainLinkMap.erase(it);
							LeaveCriticalSection(&g_csMainLinkMap);
							FreeHandleData(pMainLink->pCPPair);
							FreeHandleData(pMainLink);
						}
						FreeIoData(pPerIoData);
					}
					break;

				case OP_ZERO_RECV_FOR_MAIN_LINK_EXIT:
					{
						PPER_HANDLE_DATA pMainLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						EnterCriticalSection(&g_csCountOfMLink);
						_InterlockedDecrement(&g_dwCountOfMLink);
						LeaveCriticalSection(&g_csCountOfMLink);
						LSCloseSocket_cs(pMainLink->pCPPair);
						LSCloseSocket_cs(pMainLink);
						if (_InterlockedDecrement(&pMainLink->nRefCount) < 0)
						{
							EnterCriticalSection(&g_csMainLinkMap);
							SS_MAP_TYPE::iterator it = g_MainLinkMap.find(pMainLink->cHostId);
							if (it != g_MainLinkMap.end())
								if (it->second == pMainLink)
									g_MainLinkMap.erase(it);
							LeaveCriticalSection(&g_csMainLinkMap);
							FreeHandleData(pMainLink->pCPPair);
							FreeHandleData(pMainLink);
						}
						FreeIoData(pPerIoData);
					}
					break;

				case OP_ZERO_RECV_FOR_MAIN_LINK_EXIT_EX:
					{
						PPER_HANDLE_DATA pMainLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pMainLink);
						FreeHandleData(pMainLink);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_ZERO_RECV_FOR_REDAIL_EXIT:
					{
						PPER_HANDLE_DATA pRedailLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pRedailLink);
						EnterCriticalSection(&g_csRedailLinkMap);
						if (_InterlockedDecrement(&pRedailLink->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_RedailLinkMap.find(pRedailLink->cHostId);
							if (it != g_RedailLinkMap.end())
								if (it->second == pRedailLink)
									g_RedailLinkMap.erase(it);
							FreeHandleData(pRedailLink);
						}
						LeaveCriticalSection(&g_csRedailLinkMap);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_ACCEPT_CLIENT_LINK:
					{
						PPER_HANDLE_DATA pListener = PPER_HANDLE_DATA(pPerIoData->pOwner);
						if (_InterlockedExchangeAdd(&pListener->pCPPair->nRefCount, 0) < 8)
						{
							_InterlockedIncrement(&pListener->pCPPair->nRefCount);
							PPER_HANDLE_DATA pNewClientLink = AllocHandleData(DT_CLIENT_LINK);
							PPER_IO_DATA pNewClientLinkIo = AllocIoData();
							if (NULL == pNewClientLink || NULL == pNewClientLinkIo)
							{
								_InterlockedDecrement(&pListener->pCPPair->nRefCount);
								LSCloseSocket(pPerIoData);
								if (!PostAccept(pPerIoData, OP_ACCEPT_CLIENT_LINK))
								{
									SS_TEXT("!!!OP_ACCEPT_CLIENT_LINK：重新投递重播失败, ErrCode = %d", WSAGetLastError());
									LSCloseSocket_cs(pListener->pCPPair);
									LSCloseSocket_cs(pListener);
									if (_InterlockedDecrement(&pListener->pCPPair->nRefCount) < 0)
									{
										EnterCriticalSection(&g_csMainLinkMap);
										SS_MAP_TYPE::iterator it = g_MainLinkMap.find(pListener->pCPPair->cHostId);
										if (it != g_MainLinkMap.end())
											if (it->second == pListener->pCPPair)
												g_MainLinkMap.erase(it);
										LeaveCriticalSection(&g_csMainLinkMap);
										FreeHandleData(pListener->pCPPair);
										FreeHandleData(pListener);
									}
									FreeIoData(pPerIoData);
								}
								FreeHandleData(pNewClientLink);
								FreeIoData(pNewClientLinkIo);
								break;
							}
							pNewClientLink->s_LinkSock = pPerIoData->s_AcceptSock;
							pNewClientLink->pHelper = pListener->pCPPair;
							pNewClientLinkIo->pOwner = pNewClientLink;

							if (!PostAccept(pPerIoData, OP_ACCEPT_CLIENT_LINK))
							{
								SS_TEXT("!!!OP_ACCEPT_CLIENT_LINK：重新投递重播失败, ErrCode = %d", WSAGetLastError());
								_InterlockedDecrement(&pListener->pCPPair->nRefCount);
								LSCloseSocket_cs(pListener->pCPPair);
								LSCloseSocket_cs(pListener);
								if (_InterlockedDecrement(&pListener->pCPPair->nRefCount) < 0)
								{
									EnterCriticalSection(&g_csMainLinkMap);
									SS_MAP_TYPE::iterator it = g_MainLinkMap.find(pListener->pCPPair->cHostId);
									if (it != g_MainLinkMap.end())
										if (it->second == pListener->pCPPair)
											g_MainLinkMap.erase(it);
									LeaveCriticalSection(&g_csMainLinkMap);
									FreeHandleData(pListener->pCPPair);
									FreeHandleData(pListener);
								}
								FreeIoData(pPerIoData);
								LSCloseSocket_cs(pNewClientLink);
								FreeHandleData(pNewClientLink);
								FreeIoData(pNewClientLinkIo);
								break;
							}

							if (NULL == CreateIoCompletionPort((HANDLE)pNewClientLink->s_LinkSock, hIocp, (ULONG_PTR)pNewClientLink, 0))
							{
								// 绑定完成端口失败
								LSCloseSocket_cs(pListener->pCPPair);
								LSCloseSocket_cs(pListener);
								if (_InterlockedDecrement(&pListener->pCPPair->nRefCount) < 0)
								{
									EnterCriticalSection(&g_csMainLinkMap);
									SS_MAP_TYPE::iterator it = g_MainLinkMap.find(pListener->pCPPair->cHostId);
									if (it != g_MainLinkMap.end())
										if (it->second == pListener->pCPPair)
											g_MainLinkMap.erase(it);
									LeaveCriticalSection(&g_csMainLinkMap);
									FreeHandleData(pListener->pCPPair);
									FreeHandleData(pListener);
								}
								LSCloseSocket_cs(pNewClientLink);
								FreeHandleData(pNewClientLink);
								FreeIoData(pNewClientLinkIo);
								break;
							}

							if (!ss_dispose_client_link(pNewClientLink, pNewClientLinkIo, hIocp))
							{
								LSCloseSocket_cs(pListener->pCPPair);
								LSCloseSocket_cs(pListener);
								if (_InterlockedDecrement(&pListener->pCPPair->nRefCount) < 0)
								{
									EnterCriticalSection(&g_csMainLinkMap);
									SS_MAP_TYPE::iterator it = g_MainLinkMap.find(pListener->pCPPair->cHostId);
									if (it != g_MainLinkMap.end())
										if (it->second == pListener->pCPPair)
											g_MainLinkMap.erase(it);
									LeaveCriticalSection(&g_csMainLinkMap);
									FreeHandleData(pListener->pCPPair);
									FreeHandleData(pListener);
								}
							}
						}else
						{
							LSCloseSocket(pPerIoData);
							if (!PostAccept(pPerIoData, OP_ACCEPT_CLIENT_LINK))
							{
								SS_TEXT("!!!OP_ACCEPT_CLIENT_LINK：重新投递重播失败, ErrCode = %d", WSAGetLastError());
								LSCloseSocket_cs(pListener);
								LSCloseSocket_cs(pListener->pCPPair);
								if (_InterlockedDecrement(&pListener->pCPPair->nRefCount) < 0)
								{
									EnterCriticalSection(&g_csMainLinkMap);
									SS_MAP_TYPE::iterator it = g_MainLinkMap.find(pListener->pCPPair->cHostId);
									if (it != g_MainLinkMap.end())
										if (it->second == pListener->pCPPair)
											g_MainLinkMap.erase(it);
									LeaveCriticalSection(&g_csMainLinkMap);
									FreeHandleData(pListener->pCPPair);
									FreeHandleData(pListener);
								}
								FreeIoData(pPerIoData);
							}
						}
					}
					break;

				case OP_ACCEPT_PROXY_LINK:
					{
						PPER_HANDLE_DATA pListener = PPER_HANDLE_DATA(pPerIoData->pOwner);
						PPER_HANDLE_DATA pNewProxyLink = AllocHandleData(DT_PROXY_LINK);
						PPER_IO_DATA pNewProxyLinkIo = AllocIoData();
						if (NULL == pNewProxyLink || NULL == pNewProxyLinkIo)
						{
							LSCloseSocket(pPerIoData);
							FreeIoData(pPerIoData);		
							LSCloseSocket_cs(pListener);
							LSCloseSocket_cs(pListener->pCPPair);
							EnterCriticalSection(&g_csListenMap);
							if (_InterlockedDecrement(&pListener->nRefCount) < 0)
							{
								SS_MAP_TYPE::iterator it = g_ListenMap.find(pListener->cKey);
								if (it != g_ListenMap.end())
								{
									g_ListenMap.erase(it);
								}
								FreeHandleData(pListener->pCPPair);
								FreeHandleData(pListener);
							}					
							LeaveCriticalSection(&g_csListenMap);
							FreeHandleData(pNewProxyLink);
							FreeIoData(pNewProxyLinkIo);
							break;
						}
						pNewProxyLink->s_LinkSock = pPerIoData->s_AcceptSock;

						pNewProxyLink->pCPPair = pListener->pCPPair;
						pNewProxyLink->pCPPair->pCPPair = pNewProxyLink;

						FreeIoData(pPerIoData);		
						LSCloseSocket_cs(pListener);
						EnterCriticalSection(&g_csListenMap);
						pListener->pCPPair = NULL; //防止定时器中被释放
						if (_InterlockedDecrement(&pListener->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ListenMap.find(pListener->cKey);
							if (it != g_ListenMap.end())
							{
								g_ListenMap.erase(it);
							}
							FreeHandleData(pListener);
						}					
						LeaveCriticalSection(&g_csListenMap);

						if (NULL == CreateIoCompletionPort((HANDLE)pNewProxyLink->s_LinkSock, hIocp, (ULONG_PTR)pNewProxyLink, 0))
						{
							// 绑定完成端口失败
							LSCloseSocket_cs(pNewProxyLink);
							LSCloseSocket_cs(pNewProxyLink->pCPPair);
							FreeHandleData(pNewProxyLink->pCPPair);
							FreeHandleData(pNewProxyLink);
							FreeIoData(pNewProxyLinkIo);
							break;
						}

						// 模板
						getguid(pNewProxyLink->cKey);
						pNewProxyLink->dwThePoll = GetTickCount();
						EnterCriticalSection(&g_csProxyMap);
						g_ProxyMap.insert(SS_MAP_TYPE::value_type(pNewProxyLink->cKey, pNewProxyLink));
						LeaveCriticalSection(&g_csProxyMap);

						// 投递接收clientlink发送来的信息
						pNewProxyLinkIo->not = OP_RECV_FORM_CLIENT_LINK;
						if (!PostZeroByteRecv(pNewProxyLink->pCPPair, pNewProxyLinkIo, OP_ZERO_RECV))
						{
							LSCloseSocket_cs(pNewProxyLink);
							LSCloseSocket_cs(pNewProxyLink->pCPPair);
							EnterCriticalSection(&g_csProxyMap);
							if (_InterlockedDecrement(&pNewProxyLink->nRefCount) < 0)
							{
								SS_MAP_TYPE::iterator it = g_ProxyMap.find(pNewProxyLink->cKey);
								if (it != g_ProxyMap.end())
									g_ProxyMap.erase(it);
								FreeHandleData(pNewProxyLink->pCPPair);
								FreeHandleData(pNewProxyLink);
							}
							LeaveCriticalSection(&g_csProxyMap);
							FreeIoData(pNewProxyLinkIo);
						}
					}
					break;

				case OP_RECV_FORM_CLIENT_LINK:
					{
						LSCloseSocket_cs(pPerHandleData);
						LSCloseSocket_cs(pPerHandleData->pCPPair);
						EnterCriticalSection(&g_csProxyMap);
						if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
							if (it != g_ProxyMap.end())
								g_ProxyMap.erase(it);
							FreeHandleData(pPerHandleData->pCPPair);
							FreeHandleData(pPerHandleData);
						}
						LeaveCriticalSection(&g_csProxyMap);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_SEDN_CLIENT_INFO_2_PROXY:
					{
						LSCloseSocket_cs(pPerHandleData);
						LSCloseSocket_cs(pPerHandleData->pCPPair);
						EnterCriticalSection(&g_csProxyMap);
						if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->cKey);
							if (it != g_ProxyMap.end())
								g_ProxyMap.erase(it);
							FreeHandleData(pPerHandleData->pCPPair);
							FreeHandleData(pPerHandleData);
						}
						LeaveCriticalSection(&g_csProxyMap);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_RECV_FROM_PROXY_LINK:
					{
						LSCloseSocket_cs(pPerHandleData);
						LSCloseSocket_cs(pPerHandleData->pCPPair);
						EnterCriticalSection(&g_csProxyMap);
						if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->cKey);
							if (it != g_ProxyMap.end())
								g_ProxyMap.erase(it);
							FreeHandleData(pPerHandleData->pCPPair);
							FreeHandleData(pPerHandleData);
						}
						LeaveCriticalSection(&g_csProxyMap);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_SEND_PROXY_INFO_2_CLIENT:
					{
						LSCloseSocket_cs(pPerHandleData);
						LSCloseSocket_cs(pPerHandleData->pCPPair);
						EnterCriticalSection(&g_csProxyMap);
						if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
							if (it != g_ProxyMap.end())
								g_ProxyMap.erase(it);
							FreeHandleData(pPerHandleData->pCPPair);
							FreeHandleData(pPerHandleData);
						}
						LeaveCriticalSection(&g_csProxyMap);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_RECV_FROM_CLIENT_CONNECT:
					{
						LSCloseSocket_cs(pPerHandleData);
						LSCloseSocket_cs(pPerHandleData->pCPPair);
						EnterCriticalSection(&g_csProxyMap);
						if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
							if (it != g_ProxyMap.end())
								g_ProxyMap.erase(it);
							FreeHandleData(pPerHandleData->pCPPair);
							FreeHandleData(pPerHandleData);
						}
						LeaveCriticalSection(&g_csProxyMap);

						FreeIoData(pPerIoData);
					}
					break;

				case OP_RECV_FROM_PROXY_CONNECT:
					{
						LSCloseSocket_cs(pPerHandleData);
						LSCloseSocket_cs(pPerHandleData->pCPPair);
						EnterCriticalSection(&g_csProxyMap);
						if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->cKey);
							if (it != g_ProxyMap.end())
								g_ProxyMap.erase(it);
							FreeHandleData(pPerHandleData->pCPPair);
							FreeHandleData(pPerHandleData);
						}
						LeaveCriticalSection(&g_csProxyMap);

						FreeIoData(pPerIoData);
					}
					break;

				case OP_SEND_CLIENT_INFO_2_PROXY_CONNECT:
					{
						LSCloseSocket_cs(pPerHandleData);
						LSCloseSocket_cs(pPerHandleData->pCPPair);
						EnterCriticalSection(&g_csProxyMap);
						if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->cKey);
							if (it != g_ProxyMap.end())
								g_ProxyMap.erase(it);
							FreeHandleData(pPerHandleData->pCPPair);
							FreeHandleData(pPerHandleData);
						}
						LeaveCriticalSection(&g_csProxyMap);

						FreeIoData(pPerIoData);
					}
					break;

				case OP_SEND_PROXY_INFO_2_CLIENT_CONNECT:
					{
						LSCloseSocket_cs(pPerHandleData);
						LSCloseSocket_cs(pPerHandleData->pCPPair);
						EnterCriticalSection(&g_csProxyMap);
						if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
							if (it != g_ProxyMap.end())
								g_ProxyMap.erase(it);
							FreeHandleData(pPerHandleData->pCPPair);
							FreeHandleData(pPerHandleData);
						}
						LeaveCriticalSection(&g_csProxyMap);

						FreeIoData(pPerIoData);
					}
					break;

				case OP_ACCEPT_REDAIL_LINK:
					{
						PPER_HANDLE_DATA pNewRedailData = AllocHandleData(DT_REDAIL_LINK);
						PPER_IO_DATA pNewRedailIo = AllocIoData();
						if (NULL == pNewRedailData || NULL == pNewRedailIo)
						{
							LSCloseSocket(pPerIoData);
							if (!PostAccept(pPerIoData, OP_ACCEPT_REDAIL_LINK))
							{
								SS_TEXT("!!!OP_ACCEPT_REDAIL_LINK：重新投递重播失败, ErrCode = %d", WSAGetLastError());
								FreeIoData(pPerIoData);
							}
							FreeHandleData(pNewRedailData);
							FreeIoData(pNewRedailIo);
							break;
						}
						pNewRedailData->s_LinkSock = pPerIoData->s_AcceptSock;
						pNewRedailIo->pOwner = pNewRedailData;

						if (!PostAccept(pPerIoData, OP_ACCEPT_REDAIL_LINK))
						{
							SS_TEXT("!!!OP_ACCEPT_REDAIL_LINK：重新投递重播失败, ErrCode = %d", WSAGetLastError());
							FreeIoData(pPerIoData);
						}

						if (NULL == CreateIoCompletionPort((HANDLE)pNewRedailData->s_LinkSock, hIocp, (ULONG_PTR)pNewRedailData, 0))
						{
							LSCloseSocket_cs(pNewRedailData);
							FreeHandleData(pNewRedailData);
							FreeIoData(pNewRedailIo);
							break;
						}

						pNewRedailIo->not = OP_RECV_FROM_REDAIL_LINK;
						if (!PostZeroByteRecv(pNewRedailData, pNewRedailIo, OP_ZERO_RECV))
						{
							LSCloseSocket_cs(pNewRedailData);
							FreeHandleData(pNewRedailData);
							FreeIoData(pNewRedailIo);
						}
					}
					break;

				case OP_RECV_FROM_REDAIL_LINK:
					{
						PPER_HANDLE_DATA pRedailLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pRedailLink);
						FreeHandleData(pRedailLink);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_SEND_REDAIL_INFO_2_PROXY:
					{
						PPER_HANDLE_DATA pRedailLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pRedailLink->pHelper);
						EnterCriticalSection(&g_csRedailLinkMap);
						if (_InterlockedDecrement(&pRedailLink->pHelper->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_RedailLinkMap.find(pRedailLink->pHelper->cHostId);
							if (it != g_RedailLinkMap.end())
								if (it->second == pRedailLink->pHelper)
									g_RedailLinkMap.erase(it);
							FreeHandleData(pRedailLink->pHelper);
						}
						LeaveCriticalSection(&g_csRedailLinkMap);
						LSCloseSocket_cs(pRedailLink);
						FreeHandleData(pRedailLink);
						//	FreeHandleData(pPerHandleData);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_SEND_OK_INFO_2_CLIENT:
					{
						LSCloseSocket_cs(pPerHandleData);
						FreeHandleData(pPerHandleData);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_SEND_YESNO_2_MAIN_LINK:
					{
						PPER_HANDLE_DATA pMainLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pMainLink);
						FreeHandleData(pMainLink);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_ACCEPT_HB_LINK:
					{
						PPER_HANDLE_DATA pNewHBData = AllocHandleData(DT_HB_LINK);
						PPER_IO_DATA pNewHBIo = AllocIoData();
						if (NULL == pNewHBData || NULL == pNewHBIo)
						{
							LSCloseSocket(pPerIoData);
							if (!PostAccept(pPerIoData, OP_ACCEPT_HB_LINK))
							{
								SS_TEXT("!!!OP_ACCEPT_HB_LINK：投递心跳接收失败, ErrCode = %d", WSAGetLastError());
								FreeIoData(pPerIoData);
							}
							FreeHandleData(pNewHBData);
							FreeIoData(pNewHBIo);
							break;
						}
						pNewHBData->s_LinkSock = pPerIoData->s_AcceptSock;
						pNewHBIo->pOwner = pNewHBData;

						if (!PostAccept(pPerIoData, OP_ACCEPT_HB_LINK))
						{
							SS_TEXT("!!!OP_ACCEPT_HB_LINK：投递心跳接收失败, ErrCode = %d", WSAGetLastError());
							FreeIoData(pPerIoData);
						}

						if (NULL == CreateIoCompletionPort((HANDLE)pNewHBData->s_LinkSock, hIocp, (ULONG_PTR)pNewHBData, 0))
						{
							LSCloseSocket_cs(pNewHBData);
							FreeHandleData(pNewHBData);
							FreeIoData(pNewHBIo);
							break;
						}

						pNewHBIo->not = OP_RECV_FROM_HB_LINK;
						if (!PostZeroByteRecv(pNewHBData, pNewHBIo, OP_ZERO_RECV))
						{
							LSCloseSocket_cs(pNewHBData);
							FreeHandleData(pNewHBData);
							FreeIoData(pNewHBIo);
						}
					}
					break;

				case OP_RECV_FROM_HB_LINK:
					{
						PPER_HANDLE_DATA pHBLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pHBLink);
						FreeHandleData(pHBLink);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_RECV_FROM_HB_LINK2:
					{
						PPER_HANDLE_DATA pHBLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pHBLink);
						EnterCriticalSection(&g_csHBMap);
						if (_InterlockedDecrement(&pHBLink->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_HBMap.find(pHBLink->cHostId);
							if (it != g_HBMap.end())
							{
								if (it->second == pHBLink)
									g_HBMap.erase(it);
							}
							FreeHandleData(pHBLink);
						}
						LeaveCriticalSection(&g_csHBMap);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_RELEASE_TIMEOUT_HB:
					{
						PPER_HANDLE_DATA pHBOut = PPER_HANDLE_DATA(pPerIoData->pOwner);
						FreeIoData(pPerIoData);
						EnterCriticalSection(&g_csMainLinkMap);
						SS_MAP_TYPE::iterator it = g_MainLinkMap.find(pHBOut->cHostId);
						if (it != g_MainLinkMap.end())
						{
							if ((GetTickCount() - it->second->dwThePoll) < 20*1000)
							{
								LeaveCriticalSection(&g_csMainLinkMap);
								if (_InterlockedDecrement(&pHBOut->nRefCount) < 0)
									FreeHandleData(pHBOut);
								break;
							}
							LSCloseSocket_cs(it->second->pCPPair);
							LSCloseSocket_cs(it->second);
							g_MainLinkMap.erase(it);
						}
						LeaveCriticalSection(&g_csMainLinkMap);
						EnterCriticalSection(&g_csRedailLinkMap);
						SS_MAP_TYPE::iterator itr = g_RedailLinkMap.find(pHBOut->cHostId);
						if (itr != g_RedailLinkMap.end())
						{
							LSCloseSocket_cs(itr->second);
						}
						LeaveCriticalSection(&g_csRedailLinkMap);
						if (_InterlockedDecrement(&pHBOut->nRefCount) < 0)
							FreeHandleData(pHBOut);
					}
					break;

				case OP_RELEASE_TIMEOUT_PROXY:
					{
						PPER_HANDLE_DATA pProxyOut = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pProxyOut);
						LSCloseSocket_cs(pProxyOut->pCPPair);
						EnterCriticalSection(&g_csProxyMap);
						if (_InterlockedDecrement(&pProxyOut->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ProxyMap.find(pProxyOut->cKey);
							if (it != g_ProxyMap.end())
								g_ProxyMap.erase(it);
							FreeHandleData(pProxyOut->pCPPair);
							FreeHandleData(pProxyOut);
						}
						LeaveCriticalSection(&g_csProxyMap);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_RELEASE_TIMEOUT_LISTEN:
					{
						PPER_HANDLE_DATA pListenerOut = PPER_HANDLE_DATA(pPerIoData->pOwner);
						LSCloseSocket_cs(pListenerOut->pCPPair);
						LSCloseSocket_cs(pListenerOut);
						EnterCriticalSection(&g_csListenMap);
						if (_InterlockedDecrement(&pListenerOut->nRefCount) < 0)
						{
							SS_MAP_TYPE::iterator it = g_ListenMap.find(pListenerOut->cKey);
							if (it != g_ListenMap.end())
							{
								g_ListenMap.erase(it);							
							}
							FreeHandleData(pListenerOut->pCPPair);
							FreeHandleData(pListenerOut);
						}
						LeaveCriticalSection(&g_csListenMap);
						FreeIoData(pPerIoData);
					}
					break;

				case OP_SEND_PROXY_LISTEN_PORT_2_PROXY:
					{
						PPER_HANDLE_DATA pMainLink = PPER_HANDLE_DATA(pPerIoData->pOwner)->pHelper;
						LSCloseSocket_cs(pMainLink->pCPPair);
						LSCloseSocket_cs(pMainLink);
						//LSCloseSocket_cs(pPerHandleData->pHelper);
						//LSCloseSocket_cs(pPerHandleData->pHelper->pCPPair);
						FreeIoData(pPerIoData);

						if (_InterlockedDecrement(&pMainLink->nRefCount) < 0)
						{
							EnterCriticalSection(&g_csMainLinkMap);
							SS_MAP_TYPE::iterator it = g_MainLinkMap.find(pMainLink->cHostId);
							if (it != g_MainLinkMap.end())
								if (it->second == pMainLink)
									g_MainLinkMap.erase(it);
							LeaveCriticalSection(&g_csMainLinkMap);
							FreeHandleData(pMainLink->pCPPair);
							FreeHandleData(pMainLink);// 释放mianlink,需要引用同步
						}
					}
					break;
				default:
					break;
				}
			}else
			{
				switch (pPerIoData->ot)
				{
				case OP_RECV_FROM_MAIN_LINK:
					{
						PPER_HANDLE_DATA pMainLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						pPerIoData->nAlreadyRecvBytes += dwTrans;
						unsigned long ulWaitBytes = 0;
						if (SOCKET_ERROR != ioctlsocket(pMainLink->s_LinkSock, FIONREAD, &ulWaitBytes))
						{
							if (ulWaitBytes > 0)
							{
								pPerIoData->not = OP_RECV_FROM_MAIN_LINK;
								if (!PostZeroByteRecv(pMainLink, pPerIoData, OP_ZERO_RECV))
								{
									LSCloseSocket_cs(pMainLink);
									FreeHandleData(pMainLink);
									FreeIoData(pPerIoData);
								}
							}else
							{
								if (!ss_verify_main_link(pMainLink, pPerIoData, hIocp))
								{

								}
							}
						}else
						{
							LSCloseSocket_cs(pMainLink);
							FreeHandleData(pMainLink);
							FreeIoData(pPerIoData);
						}
					}
					break;

				case OP_SEND_CLIENT_LISTEN_PORT_2_PROXY:
					{
						PPER_HANDLE_DATA pMainLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						pPerIoData->nAlreadySendBytes += dwTrans;
						if (pPerIoData->nAlreadyRecvBytes > pPerIoData->nAlreadySendBytes)
						{
							if (!PostSend(pMainLink, pPerIoData, OP_SEND_CLIENT_LISTEN_PORT_2_PROXY))
							{
								EnterCriticalSection(&g_csCountOfMLink);
								_InterlockedDecrement(&g_dwCountOfMLink);
								LeaveCriticalSection(&g_csCountOfMLink);
								LSCloseSocket_cs(pMainLink->pCPPair);
								LSCloseSocket_cs(pMainLink);
								if (_InterlockedDecrement(&pMainLink->nRefCount) < 0)
								{
									EnterCriticalSection(&g_csMainLinkMap);
									SS_MAP_TYPE::iterator it = g_MainLinkMap.find(pMainLink->cHostId);
									if (it != g_MainLinkMap.end())
										if (it->second == pMainLink)
											g_MainLinkMap.erase(it);
									LeaveCriticalSection(&g_csMainLinkMap);
									FreeHandleData(pMainLink->pCPPair);
									FreeHandleData(pMainLink);
								}
								FreeIoData(pPerIoData);
							}
						}else
						{
							pPerIoData->nAlreadyRecvBytes = pPerIoData->nAlreadySendBytes = 0;
							if (!PostZeroByteRecv(pMainLink, pPerIoData, OP_ZERO_RECV_FOR_MAIN_LINK_EXIT))
							{
								EnterCriticalSection(&g_csCountOfMLink);
								_InterlockedDecrement(&g_dwCountOfMLink);
								LeaveCriticalSection(&g_csCountOfMLink);
								LSCloseSocket_cs(pMainLink->pCPPair);
								LSCloseSocket_cs(pMainLink);
								if (_InterlockedDecrement(&pMainLink->nRefCount) < 0)
								{
									EnterCriticalSection(&g_csMainLinkMap);
									SS_MAP_TYPE::iterator it = g_MainLinkMap.find(pMainLink->cHostId);
									if (it != g_MainLinkMap.end())
										if (it->second == pMainLink)
											g_MainLinkMap.erase(it);
									LeaveCriticalSection(&g_csMainLinkMap);
									FreeHandleData(pMainLink->pCPPair);
									FreeHandleData(pMainLink);
								}
								FreeIoData(pPerIoData);
								break;
							}
						}
					}
					break;

				case OP_RECV_FORM_CLIENT_LINK:
					{
						pPerIoData->nAlreadyRecvBytes += dwTrans;
						if (strncmp("GET ", pPerIoData->buf, 4) == 0)
						{
							// 3亿次 strncmp 1.7s  strstr 2.5s release模式
							if (strncmp("\r\n\r\n", pPerIoData->buf+pPerIoData->nAlreadyRecvBytes-4, 4) != 0)
						//	if (strstr(pPerIoData->buf, "\r\n\r\n") == NULL)
							{
								pPerIoData->not = OP_RECV_FORM_CLIENT_LINK;
								if (!PostZeroByteRecv(pPerHandleData, pPerIoData, OP_ZERO_RECV))
								{
									LSCloseSocket_cs(pPerHandleData);
									LSCloseSocket_cs(pPerHandleData->pCPPair);
									EnterCriticalSection(&g_csProxyMap);
									if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
									{
										SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
										if (it != g_ProxyMap.end())
											g_ProxyMap.erase(it);									
										FreeHandleData(pPerHandleData->pCPPair);
										FreeHandleData(pPerHandleData);
									}
									LeaveCriticalSection(&g_csProxyMap);
									FreeIoData(pPerIoData);
									break;
								}
								break;
							}
						}else
						{
							pPerIoData->bIsConnect = TRUE;
						}
						if (!PostSend(pPerHandleData->pCPPair, pPerIoData, OP_SEDN_CLIENT_INFO_2_PROXY))
						{
							LSCloseSocket_cs(pPerHandleData);
							LSCloseSocket_cs(pPerHandleData->pCPPair);
							EnterCriticalSection(&g_csProxyMap);
							if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
							{
								SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
								if (it != g_ProxyMap.end())
									g_ProxyMap.erase(it);
								FreeHandleData(pPerHandleData->pCPPair);
								FreeHandleData(pPerHandleData);
							}
							LeaveCriticalSection(&g_csProxyMap);
							FreeIoData(pPerIoData);
						}

						/*unsigned long ulWaitBytes = 0;
						if (SOCKET_ERROR != ioctlsocket(pPerHandleData->s_LinkSock, FIONREAD, &ulWaitBytes))
						{
							if (ulWaitBytes > 0 && pPerIoData->nAlreadyRecvBytes < SS_MAX_DATA)
							{
								pPerIoData->not = OP_RECV_FORM_CLIENT_LINK;
								if (!PostZeroByteRecv(pPerHandleData, pPerIoData, OP_ZERO_RECV))
								{
									LSCloseSocket_cs(pPerHandleData);
									LSCloseSocket_cs(pPerHandleData->pCPPair);
									EnterCriticalSection(&g_csProxyMap);
									if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
									{
										SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
										if (it != g_ProxyMap.end())
											g_ProxyMap.erase(it);									
										FreeHandleData(pPerHandleData->pCPPair);
										FreeHandleData(pPerHandleData);
									}
									LeaveCriticalSection(&g_csProxyMap);
									FreeIoData(pPerIoData);
								}
							}else
							{
								if (strncmp("CONNECT ", pPerIoData->buf, 8) == 0 
									|| pPerIoData->nAlreadyRecvBytes >= SS_MAX_DATA
									|| strncmp("/test_up_spd", pPerIoData->buf+5, 12) == 0)
								{
									pPerIoData->bIsConnect = TRUE;
								}
								if (!PostSend(pPerHandleData->pCPPair, pPerIoData, OP_SEDN_CLIENT_INFO_2_PROXY))
								{
									LSCloseSocket_cs(pPerHandleData);
									LSCloseSocket_cs(pPerHandleData->pCPPair);
									EnterCriticalSection(&g_csProxyMap);
									if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
									{
										SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
										if (it != g_ProxyMap.end())
											g_ProxyMap.erase(it);
										FreeHandleData(pPerHandleData->pCPPair);
										FreeHandleData(pPerHandleData);
									}
									LeaveCriticalSection(&g_csProxyMap);
									FreeIoData(pPerIoData);
								}
							}
						}else
						{
							LSCloseSocket_cs(pPerHandleData);
							LSCloseSocket_cs(pPerHandleData->pCPPair);
							EnterCriticalSection(&g_csProxyMap);
							if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
							{
								SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
								if (it != g_ProxyMap.end())
									g_ProxyMap.erase(it);
								FreeHandleData(pPerHandleData->pCPPair);
								FreeHandleData(pPerHandleData);
							}
							LeaveCriticalSection(&g_csProxyMap);
							FreeIoData(pPerIoData);
						}*/
					}
					break;

				case OP_SEND_PROXY_LISTEN_PORT_2_PROXY:
					{
						PPER_HANDLE_DATA pMainLink = PPER_HANDLE_DATA(pPerIoData->pOwner)->pHelper;
						pPerIoData->nAlreadySendBytes += dwTrans;
						if (pPerIoData->nAlreadyRecvBytes > pPerIoData->nAlreadySendBytes)
						{
							if (!PostSend(pMainLink, pPerIoData, OP_SEND_PROXY_LISTEN_PORT_2_PROXY))
							{
								LSCloseSocket_cs(pMainLink->pCPPair);
								LSCloseSocket_cs(pMainLink);
								//LSCloseSocket_cs(pPerHandleData->pHelper);
								//LSCloseSocket_cs(pPerHandleData->pHelper->pCPPair);
								FreeIoData(pPerIoData);

								if (_InterlockedDecrement(&pMainLink->nRefCount) < 0)
								{
									EnterCriticalSection(&g_csMainLinkMap);
									SS_MAP_TYPE::iterator it = g_MainLinkMap.find(pMainLink->cHostId);
									if (it != g_MainLinkMap.end())
										if (it->second == pMainLink)
											g_MainLinkMap.erase(it);
									LeaveCriticalSection(&g_csMainLinkMap);
									FreeHandleData(pMainLink->pCPPair);
									FreeHandleData(pMainLink);
								}
							}
						}else
						{
							FreeIoData(pPerIoData);

							if (_InterlockedDecrement(&pMainLink->nRefCount) < 0)
							{
								EnterCriticalSection(&g_csMainLinkMap);
								SS_MAP_TYPE::iterator it = g_MainLinkMap.find(pMainLink->cHostId);
								if (it != g_MainLinkMap.end())
									if (it->second == pMainLink)
										g_MainLinkMap.erase(it);
								LeaveCriticalSection(&g_csMainLinkMap);
								FreeHandleData(pMainLink->pCPPair);
								FreeHandleData(pMainLink);
							}
						}
					}
					break;

				case OP_SEDN_CLIENT_INFO_2_PROXY:
					{
						pPerIoData->nAlreadySendBytes += dwTrans;
						if (pPerIoData->nAlreadyRecvBytes > pPerIoData->nAlreadySendBytes)
						{
							if (!PostSend(pPerHandleData, pPerIoData, OP_SEDN_CLIENT_INFO_2_PROXY))
							{
								LSCloseSocket_cs(pPerHandleData);
								LSCloseSocket_cs(pPerHandleData->pCPPair);
								EnterCriticalSection(&g_csProxyMap);
								if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
								{
									SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->cKey);
									if (it != g_ProxyMap.end())
										g_ProxyMap.erase(it);
									FreeHandleData(pPerHandleData->pCPPair);
									FreeHandleData(pPerHandleData);
								}
								LeaveCriticalSection(&g_csProxyMap);
								FreeIoData(pPerIoData);
							}
						}else
						{
							if (pPerIoData->bIsConnect)
							{
								_InterlockedIncrement(&pPerHandleData->nRefCount);// connet请求的时候，需要c/p同时释放
								pPerIoData->not = OP_RECV_FROM_PROXY_CONNECT;
								pPerIoData->nAlreadyRecvBytes = pPerIoData->nAlreadySendBytes = 0;
								if (!PostZeroByteRecv(pPerHandleData, pPerIoData, OP_ZERO_RECV))
								{
									_InterlockedDecrement(&pPerHandleData->nRefCount);
									LSCloseSocket_cs(pPerHandleData);
									LSCloseSocket_cs(pPerHandleData->pCPPair);
									EnterCriticalSection(&g_csProxyMap);
									if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
									{
										SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->cKey);
										if (it != g_ProxyMap.end())
											g_ProxyMap.erase(it);
										FreeHandleData(pPerHandleData->pCPPair);
										FreeHandleData(pPerHandleData);
									}
									LeaveCriticalSection(&g_csProxyMap);

									FreeIoData(pPerIoData);
									break;
								}

								PPER_IO_DATA pRecvConnectIo = AllocIoData();
								pRecvConnectIo->not = OP_RECV_FROM_CLIENT_CONNECT;
								if (!PostZeroByteRecv(pPerHandleData->pCPPair, pRecvConnectIo, OP_ZERO_RECV))
								{
									LSCloseSocket_cs(pPerHandleData);
									LSCloseSocket_cs(pPerHandleData->pCPPair);
									EnterCriticalSection(&g_csProxyMap);
									if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
									{
										SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->cKey);
										if (it != g_ProxyMap.end())
											g_ProxyMap.erase(it);
										FreeHandleData(pPerHandleData->pCPPair);
										FreeHandleData(pPerHandleData);
									}
									LeaveCriticalSection(&g_csProxyMap);
									FreeIoData(pRecvConnectIo);
									break;
								}
							}else
							{
								pPerIoData->not = OP_RECV_FROM_PROXY_LINK;
								pPerIoData->nAlreadyRecvBytes = pPerIoData->nAlreadySendBytes = 0;
								if (!PostZeroByteRecv(pPerHandleData, pPerIoData, OP_ZERO_RECV))
								{
									LSCloseSocket_cs(pPerHandleData);
									LSCloseSocket_cs(pPerHandleData->pCPPair);
									EnterCriticalSection(&g_csProxyMap);
									if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
									{
										SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->cKey);
										if (it != g_ProxyMap.end())
											g_ProxyMap.erase(it);
										FreeHandleData(pPerHandleData->pCPPair);
										FreeHandleData(pPerHandleData);
									}
									LeaveCriticalSection(&g_csProxyMap);
									FreeIoData(pPerIoData);
								}
							}
						}
					}
					break;

				case OP_RECV_FROM_PROXY_LINK:
					{
						pPerHandleData->dwThePoll = GetTickCount();
						pPerIoData->nAlreadyRecvBytes = dwTrans;
						if (!PostSend(pPerHandleData->pCPPair, pPerIoData, OP_SEND_PROXY_INFO_2_CLIENT))
						{
							LSCloseSocket_cs(pPerHandleData);
							LSCloseSocket_cs(pPerHandleData->pCPPair);
							EnterCriticalSection(&g_csProxyMap);
							if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
							{
								SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->cKey);
								if (it != g_ProxyMap.end())
									g_ProxyMap.erase(it);
								FreeHandleData(pPerHandleData->pCPPair);
								FreeHandleData(pPerHandleData);
							}
							LeaveCriticalSection(&g_csProxyMap);
							FreeIoData(pPerIoData);
						}
					}
					break;

				case OP_SEND_PROXY_INFO_2_CLIENT:
					{
						pPerIoData->nAlreadySendBytes += dwTrans;
						if (pPerIoData->nAlreadyRecvBytes > pPerIoData->nAlreadySendBytes)
						{
							if (!PostSend(pPerHandleData, pPerIoData, OP_SEND_PROXY_INFO_2_CLIENT))
							{	
								LSCloseSocket_cs(pPerHandleData);
								LSCloseSocket_cs(pPerHandleData->pCPPair);
								EnterCriticalSection(&g_csProxyMap);
								if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
								{
									SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
									if (it != g_ProxyMap.end())
										g_ProxyMap.erase(it);
									FreeHandleData(pPerHandleData->pCPPair);
									FreeHandleData(pPerHandleData);
								}
								LeaveCriticalSection(&g_csProxyMap);
								FreeIoData(pPerIoData);
							}
						}else
						{
							pPerIoData->nAlreadyRecvBytes = pPerIoData->nAlreadySendBytes = 0;
							pPerIoData->not = OP_RECV_FROM_PROXY_LINK;
							if (!PostZeroByteRecv(pPerHandleData->pCPPair, pPerIoData, OP_ZERO_RECV))
							{
								LSCloseSocket_cs(pPerHandleData);
								LSCloseSocket_cs(pPerHandleData->pCPPair);
								EnterCriticalSection(&g_csProxyMap);
								if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
								{
									SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
									if (it != g_ProxyMap.end())
										g_ProxyMap.erase(it);
									FreeHandleData(pPerHandleData->pCPPair);
									FreeHandleData(pPerHandleData);
								}
								LeaveCriticalSection(&g_csProxyMap);
								FreeIoData(pPerIoData);
							}
						}
					}
					break;

				case OP_RECV_FROM_CLIENT_CONNECT:
					{
						pPerIoData->nAlreadyRecvBytes = dwTrans;
						if (!PostSend(pPerHandleData->pCPPair, pPerIoData, OP_SEND_CLIENT_INFO_2_PROXY_CONNECT))
						{
							LSCloseSocket_cs(pPerHandleData);
							LSCloseSocket_cs(pPerHandleData->pCPPair);
							EnterCriticalSection(&g_csProxyMap);
							if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
							{
								SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
								if (it != g_ProxyMap.end())
									g_ProxyMap.erase(it);
								FreeHandleData(pPerHandleData->pCPPair);
								FreeHandleData(pPerHandleData);
							}
							LeaveCriticalSection(&g_csProxyMap);

							FreeIoData(pPerIoData);
						}
					}
					break;

				case OP_SEND_CLIENT_INFO_2_PROXY_CONNECT:
					{
						pPerIoData->nAlreadySendBytes += dwTrans;
						if (pPerIoData->nAlreadyRecvBytes > pPerIoData->nAlreadySendBytes)
						{
							if (!PostSend(pPerHandleData, pPerIoData, OP_SEND_CLIENT_INFO_2_PROXY_CONNECT))
							{
								LSCloseSocket_cs(pPerHandleData);
								LSCloseSocket_cs(pPerHandleData->pCPPair);
								EnterCriticalSection(&g_csProxyMap);
								if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
								{
									SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->cKey);
									if (it != g_ProxyMap.end())
										g_ProxyMap.erase(it);
									FreeHandleData(pPerHandleData->pCPPair);
									FreeHandleData(pPerHandleData);
								}
								LeaveCriticalSection(&g_csProxyMap);

								FreeIoData(pPerIoData);
							}
						}else
						{
							pPerIoData->not = OP_RECV_FROM_CLIENT_CONNECT;
							pPerIoData->nAlreadyRecvBytes = pPerIoData->nAlreadySendBytes = 0;
							if (!PostZeroByteRecv(pPerHandleData->pCPPair, pPerIoData, OP_ZERO_RECV))
							{
								LSCloseSocket_cs(pPerHandleData);
								LSCloseSocket_cs(pPerHandleData->pCPPair);
								EnterCriticalSection(&g_csProxyMap);
								if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
								{
									SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->cKey);
									if (it != g_ProxyMap.end())
										g_ProxyMap.erase(it);
									FreeHandleData(pPerHandleData->pCPPair);
									FreeHandleData(pPerHandleData);
								}
								LeaveCriticalSection(&g_csProxyMap);

								FreeIoData(pPerIoData);
							}
						}
					}
					break;

				case OP_RECV_FROM_PROXY_CONNECT:
					{
						pPerHandleData->dwThePoll = GetTickCount();
						pPerIoData->nAlreadyRecvBytes = dwTrans;
						if (!PostSend(pPerHandleData->pCPPair, pPerIoData, OP_SEND_PROXY_INFO_2_CLIENT_CONNECT))
						{
							LSCloseSocket_cs(pPerHandleData);
							LSCloseSocket_cs(pPerHandleData->pCPPair);
							EnterCriticalSection(&g_csProxyMap);
							if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
							{
								SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->cKey);
								if (it != g_ProxyMap.end())
									g_ProxyMap.erase(it);
								FreeHandleData(pPerHandleData->pCPPair);
								FreeHandleData(pPerHandleData);
							}
							LeaveCriticalSection(&g_csProxyMap);

							FreeIoData(pPerIoData);
						}
					}
					break;

				case OP_SEND_PROXY_INFO_2_CLIENT_CONNECT:
					{
						pPerIoData->nAlreadySendBytes += dwTrans;
						if (pPerIoData->nAlreadyRecvBytes > pPerIoData->nAlreadySendBytes)
						{
							if (!PostSend(pPerHandleData, pPerIoData, OP_SEND_PROXY_INFO_2_CLIENT_CONNECT))
							{
								LSCloseSocket_cs(pPerHandleData);
								LSCloseSocket_cs(pPerHandleData->pCPPair);
								EnterCriticalSection(&g_csProxyMap);
								if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
								{									
									SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
									if (it != g_ProxyMap.end())
										g_ProxyMap.erase(it);
									FreeHandleData( pPerHandleData->pCPPair);
									FreeHandleData( pPerHandleData);
								}
								LeaveCriticalSection(&g_csProxyMap);

								FreeIoData(pPerIoData);
							}
						}else
						{
							pPerIoData->not = OP_RECV_FROM_PROXY_CONNECT;
							pPerIoData->nAlreadyRecvBytes = pPerIoData->nAlreadySendBytes = 0;
							if (!PostZeroByteRecv(pPerHandleData->pCPPair, pPerIoData, OP_ZERO_RECV))
							{
								LSCloseSocket_cs(pPerHandleData);
								LSCloseSocket_cs(pPerHandleData->pCPPair);
								EnterCriticalSection(&g_csProxyMap);
								if (_InterlockedDecrement(&pPerHandleData->pCPPair->nRefCount) < 0)
								{
									SS_MAP_TYPE::iterator it = g_ProxyMap.find(pPerHandleData->pCPPair->cKey);
									if (it != g_ProxyMap.end())
										g_ProxyMap.erase(it);
									FreeHandleData(pPerHandleData->pCPPair);
									FreeHandleData(pPerHandleData);
								}
								LeaveCriticalSection(&g_csProxyMap);
								FreeIoData(pPerIoData);
							}
						}
					}
					break;

				case OP_RECV_FROM_REDAIL_LINK:
					{
						PPER_HANDLE_DATA pRedailLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						pPerIoData->nAlreadyRecvBytes += dwTrans;
						unsigned long ulWaitBytes = 0;
						if (SOCKET_ERROR != ioctlsocket(pRedailLink->s_LinkSock, FIONREAD, &ulWaitBytes))
						{
							if (ulWaitBytes > 0)
							{
								pPerIoData->not = OP_RECV_FROM_REDAIL_LINK;
								if (!PostZeroByteRecv(pRedailLink, pPerIoData, OP_ZERO_RECV))
								{
									LSCloseSocket_cs(pRedailLink);
									FreeHandleData(pRedailLink);
									FreeIoData(pPerIoData);
								}
							}else
							{
								ss_dispose_redail_process(pRedailLink, pPerIoData);
							}
						}else
						{
							LSCloseSocket_cs(pRedailLink);
							FreeHandleData(pRedailLink);
							FreeIoData(pPerIoData);
						}
					}
					break;

				case OP_SEND_REDAIL_INFO_2_PROXY:
					{
						PPER_HANDLE_DATA pRedailLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						pPerIoData->nAlreadySendBytes += dwTrans;
						if (pPerIoData->nAlreadyRecvBytes > pPerIoData->nAlreadySendBytes)
						{
							if (!PostSend(pRedailLink->pHelper, pPerIoData, OP_SEND_REDAIL_INFO_2_PROXY))
							{
								LSCloseSocket_cs(pRedailLink->pHelper);
								EnterCriticalSection(&g_csRedailLinkMap);
								if (_InterlockedDecrement(&pRedailLink->pHelper->nRefCount) < 0)
								{
									SS_MAP_TYPE::iterator it = g_RedailLinkMap.find(pRedailLink->pHelper->cHostId);
									if (it != g_RedailLinkMap.end())
										if (it->second == pRedailLink->pHelper)
											g_RedailLinkMap.erase(it);
									FreeHandleData(pRedailLink->pHelper);
								}
								LeaveCriticalSection(&g_csRedailLinkMap);
								LSCloseSocket_cs(pRedailLink);
								FreeHandleData(pRedailLink);
								//	FreeHandleData(pPerHandleData);
								FreeIoData(pPerIoData);
							}
						}else
						{
							EnterCriticalSection(&g_csRedailLinkMap);
							if (_InterlockedDecrement(&pRedailLink->pHelper->nRefCount) < 0)
							{
								SS_MAP_TYPE::iterator it = g_RedailLinkMap.find(pRedailLink->pHelper->cHostId);
								if (it != g_RedailLinkMap.end())
									if (it->second == pRedailLink->pHelper)
										g_RedailLinkMap.erase(it);
								FreeHandleData(pRedailLink->pHelper);
							}
							LeaveCriticalSection(&g_csRedailLinkMap);
							ss_dispose_ok_process(pRedailLink, pPerIoData);
							//if (_InterlockedDecrement(&pPerHandleData->nRefCount) < 0)
							//	FreeHandleData(pPerHandleData);
						}
					}
					break;

				case OP_SEND_OK_INFO_2_CLIENT:
					{
						pPerIoData->nAlreadySendBytes += dwTrans;
						if (pPerIoData->nAlreadyRecvBytes > pPerIoData->nAlreadySendBytes)
						{
							if (!PostSend(pPerHandleData, pPerIoData, OP_SEND_OK_INFO_2_CLIENT))
							{
								LSCloseSocket_cs(pPerHandleData);
								FreeHandleData(pPerHandleData);
								FreeIoData(pPerIoData);
							}
						}else
						{
							LSCloseSocket_cs(pPerHandleData);
							FreeHandleData(pPerHandleData);
							FreeIoData(pPerIoData);
						}

					}
					break;

				case OP_SEND_YESNO_2_MAIN_LINK:
					{
						PPER_HANDLE_DATA pMainLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						pPerIoData->nAlreadySendBytes += dwTrans;
						if (pPerIoData->nAlreadyRecvBytes > pPerIoData->nAlreadySendBytes)
						{
							if (!PostSend(pMainLink, pPerIoData, OP_SEND_YESNO_2_MAIN_LINK))
							{
								LSCloseSocket_cs(pMainLink);
								FreeHandleData(pMainLink);
								FreeIoData(pPerIoData);
							}
						}else
						{
							// 投递接收mainlink发送来的信息
							pPerIoData->nAlreadyRecvBytes = pPerIoData->nAlreadySendBytes = 0;
							pPerIoData->not = OP_RECV_FROM_MAIN_LINK;
							if (!PostZeroByteRecv(pMainLink, pPerIoData, OP_ZERO_RECV))
							{
								LSCloseSocket_cs(pMainLink);
								FreeHandleData(pMainLink);
								FreeIoData(pPerIoData);
							}
						}
					}
					break;

				case OP_RECV_FROM_HB_LINK:
					{
						PPER_HANDLE_DATA pHBLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						pPerIoData->nAlreadyRecvBytes += dwTrans;
						unsigned long ulWaitBytes = 0;
						if (SOCKET_ERROR != ioctlsocket(pHBLink->s_LinkSock, FIONREAD, &ulWaitBytes))
						{
							if (ulWaitBytes > 0)
							{
								pPerIoData->not = OP_RECV_FROM_HB_LINK;
								if (!PostZeroByteRecv(pHBLink, pPerIoData, OP_ZERO_RECV))
								{
									LSCloseSocket_cs(pHBLink);
									FreeHandleData(pHBLink);
									FreeIoData(pPerIoData);
								}
							}else
							{
								ss_dispose_hb_process(pHBLink, pPerIoData);
							}
						}else
						{
							LSCloseSocket_cs(pHBLink);
							FreeHandleData(pHBLink);
							FreeIoData(pPerIoData);
						}
					}
					break;

				case OP_RECV_FROM_HB_LINK2:
					{
						PPER_HANDLE_DATA pHBLink = PPER_HANDLE_DATA(pPerIoData->pOwner);
						pPerIoData->nAlreadyRecvBytes += dwTrans;
						unsigned long ulWaitBytes = 0;
						if (SOCKET_ERROR != ioctlsocket(pHBLink->s_LinkSock, FIONREAD, &ulWaitBytes))
						{
							if (ulWaitBytes > 0)
							{
								pPerIoData->not = OP_RECV_FROM_HB_LINK2;
								if (!PostZeroByteRecv(pHBLink, pPerIoData, OP_ZERO_RECV))
								{
									LSCloseSocket_cs(pHBLink);
									EnterCriticalSection(&g_csHBMap);
									if (_InterlockedDecrement(&pHBLink->nRefCount) < 0)
									{
										SS_MAP_TYPE::iterator it = g_HBMap.find(pHBLink->cHostId);
										if (it != g_HBMap.end())
										{
											if (it->second == pHBLink)
												g_HBMap.erase(it);
										}
										FreeHandleData(pHBLink);
									}
									LeaveCriticalSection(&g_csHBMap);
									FreeIoData(pPerIoData);
								}
							}else
							{
								ss_dispose_hb_process2(pHBLink, pPerIoData);
							}
						}else
						{
							LSCloseSocket_cs(pHBLink);
							EnterCriticalSection(&g_csHBMap);
							if (_InterlockedDecrement(&pHBLink->nRefCount) < 0)
							{
								SS_MAP_TYPE::iterator it = g_HBMap.find(pHBLink->cHostId);
								if (it != g_HBMap.end())
								{
									if (it->second == pHBLink)
										g_HBMap.erase(it);
								}
								FreeHandleData(pHBLink);
							}
							LeaveCriticalSection(&g_csHBMap);
							FreeIoData(pPerIoData);
						}
					}
					break;
				default:
					break;
				}
			}
		}
	}

	return 0;
}