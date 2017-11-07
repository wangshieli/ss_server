#include "ss_global_resource.h"

LPFN_ACCEPTEX				pfnSockAcceptEx = NULL;
LPFN_GETACCEPTEXSOCKADDRS	pfnSockGetAcceptExSockAddrs = NULL;
LPFN_CONNECTEX				pfnSockConnectEx = NULL;
LPFN_DISCONNECTEX			pfnSockDisconnectEx = NULL;
LPFN_TRANSMITFILE			pfnSockTransmitFile = NULL;
LPFN_TRANSMITPACKETS		pfnSockTransmitPackets = NULL;

HANDLE hWorkerIocp = NULL;
HANDLE hAcceptIocp = NULL;

SS_MAP_TYPE g_MainLinkMap;
CRITICAL_SECTION g_csMainLinkMap;

SS_MAP_TYPE g_RedailLinkMap;
CRITICAL_SECTION g_csRedailLinkMap;

SS_MAP_TYPE g_HBMap;
CRITICAL_SECTION g_csHBMap;

SS_MAP_TYPE g_ListenMap;
CRITICAL_SECTION g_csListenMap;

SS_MAP_TYPE g_ProxyMap;
CRITICAL_SECTION g_csProxyMap;

deque<PPER_HANDLE_DATA> g_PerHandleDq;
deque<PPER_IO_DATA> g_PerIoDq;

CRITICAL_SECTION g_csPerHandleDq; 
CRITICAL_SECTION g_csPerIoDq;

unsigned int g_uiLogThreadId = 0;

long g_dwCountOfMLink = 0;
CRITICAL_SECTION g_csCountOfMLink;

DWORD				g_nSvrListenerCount = 0;
PPER_HANDLE_DATA	g_hSvrListener[WSA_MAXIMUM_WAIT_EVENTS];
HANDLE				g_hSvrListenerEvent[WSA_MAXIMUM_WAIT_EVENTS];

CRITICAL_SECTION GTcpSvrPendingAcceptCS;

PPER_HANDLE_DATA pGTcpSvrPendingAcceptHead = NULL;

vector<PPER_HANDLE_DATA> AcceptOutT;