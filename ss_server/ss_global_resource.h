#pragma once
#include <WinSock2.h>
#include <string>
#include <iostream>
#include <process.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#include <map>
#include <list>
#include <deque>
#include <vector>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

#define xmalloc(s) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (s))
#define xfree(p) HeapFree(GetProcessHeap(), 0, (p)) 

#define SS_MAX_DATA 1024 * 4
#define SS_MAX_MAIN_ACCEPT 40
#define SS_MAX_CLIENT_ACCEPT 4
#define SS_MAX_SIZE_HANDLE_DEQUE 4500
#define SS_MAX_SIZE_IO_DEQUE 5000

#define SS_MAX_MAIN_LINK 120

#define SS_MAIN_LISTEN_SOCKET "5001"
#define SS_REDAIL_LISTEN_SOCKET "6086"
#define SS_HB_LISTEN_SOCKET "5005"

#define SS_LOG_SHOW WM_USER + 1
#define SS_LOG_TEXT WM_USER + 2

extern LPFN_ACCEPTEX				pfnSockAcceptEx;
extern LPFN_GETACCEPTEXSOCKADDRS	pfnSockGetAcceptExSockAddrs;
extern LPFN_CONNECTEX				pfnSockConnectEx;
extern LPFN_DISCONNECTEX			pfnSockDisconnectEx;
extern LPFN_TRANSMITFILE			pfnSockTransmitFile;
extern LPFN_TRANSMITPACKETS			pfnSockTransmitPackets;

extern HANDLE hWorkerIocp;
extern HANDLE hAcceptIocp ;

typedef enum _op_type
{
	OP_NULL,
	OP_DO_ALL_ACCEPT,
	OP_ACCEPT_MAIN_LINK, // 投递接收代理长链接
	OP_ZERO_RECV,
	OP_RECV_FROM_MAIN_LINK,
	OP_ACCEPT_CLIENT_LINK,
	OP_ZERO_SEND,
	OP_SEND_CLIENT_LISTEN_PORT_2_PROXY,
	OP_ZERO_RECV_FOR_MAIN_LINK_EXIT,
	OP_ZERO_RECV_FOR_MAIN_LINK_EXIT_EX,
	OP_SEND_PROXY_LISTEN_PORT_2_PROXY,
	OP_ACCEPT_PROXY_LINK,
	/**********客户端、带来操作流程************/
	OP_RECV_FORM_CLIENT_LINK,
	OP_SEDN_CLIENT_INFO_2_PROXY,
	OP_RECV_FROM_PROXY_LINK,
	OP_SEND_PROXY_INFO_2_CLIENT,
	OP_RECV_FROM_CLIENT_CONNECT,
	OP_SEND_CLIENT_INFO_2_PROXY_CONNECT,
	OP_RECV_FROM_PROXY_CONNECT,
	OP_SEND_PROXY_INFO_2_CLIENT_CONNECT,
	/**********客户端、带来操作流程************/
	OP_ACCEPT_REDAIL_LINK,
	OP_RECV_FROM_REDAIL_LINK,
	OP_ZERO_RECV_FOR_REDAIL_EXIT,
	OP_SEND_REDAIL_INFO_2_PROXY,
	OP_SEND_OK_INFO_2_CLIENT,
	OP_SEND_YESNO_2_MAIN_LINK,
	OP_ACCEPT_HB_LINK,
	OP_RECV_FROM_HB_LINK,
	OP_RECV_FROM_HB_LINK2,
	OP_RELEASE_TIMEOUT_HB, // HB超时，不是客户端
	OP_RELEASE_TIMEOUT_PROXY,
	OP_RELEASE_TIMEOUT_LISTEN
}OP_TYPE;

typedef enum _device_type
{
	DT_NULL,
	DT_MAIN_LINK_LISTENER,
	DT_REDAIL_LINK_LISTENER,
	DT_HB_LINK_LISTENER,
	DT_MAIN_LINK,
	DT_LISTEN_CLIENT,
	DT_CLIENT_LINK,
	DT_LISTEN_PROXY,
	DT_PROXY_LINK,
	DT_REDAIL_LINK,
	DT_HB_LINK
}device_type;

typedef struct _per_io_data
{
public:
	void clean()
	{
		ZeroMemory(&ol, sizeof(ol));
		pOwner = NULL;
		s_ListenSock = INVALID_SOCKET;
		s_AcceptSock = INVALID_SOCKET;
		ZeroMemory(buf, SS_MAX_DATA);
		wsaBuf.buf = buf;
		wsaBuf.len = 0;
		nAlreadyRecvBytes = 0;
		nAlreadySendBytes = 0;
		ot = OP_NULL;
		not = OP_NULL;
		pOwner = NULL;
		bIsConnect = FALSE;
	}
public:
	WSAOVERLAPPED ol;
	void*	pOwner;
	SOCKET s_ListenSock;
	SOCKET s_AcceptSock;
	WSABUF wsaBuf;
	char buf[SS_MAX_DATA];
	DWORD nAlreadySendBytes;
	DWORD nAlreadyRecvBytes;
	OP_TYPE ot;
	OP_TYPE not;
	BOOL bIsConnect;
}PER_IO_DATA, *PPER_IO_DATA;

typedef struct _per_handle_data
{
public:
	void clean(device_type _dt)
	{
		pfnOnAccept = NULL;
		pfnOnAcceptErr = NULL;
		pOwner = NULL;
		pAcceptIo = NULL;
		pNext = NULL;
		pPrior = NULL;
		s_LinkSock = INVALID_SOCKET;
		pCPPair = NULL;
		pHelper = NULL;
		nRefCount = 0;
		dwThePoll = 0;
		dt = _dt;
		memset(cHostId, 0x00, 32);
		memset(cKey, 0x00, 128);
	}
public:
	void* pfnOnAccept;
	void* pfnOnAcceptErr;
	void* pOwner;
	void* pAcceptIo;
	struct _per_handle_data* pNext;
	struct _per_handle_data* pPrior;
	struct _per_handle_data* pCPPair; // 客户端/代理  mainlink/clientlisten  相互发送数据
	struct _per_handle_data* pHelper;
	SOCKET s_LinkSock;
	char cHostId[32];
	char cKey[128];
	device_type dt;
	long nRefCount;
	DWORD dwThePoll;
	CRITICAL_SECTION cs;
}PER_HANDLE_DATA, *PPER_HANDLE_DATA;

struct cmpkey
{
	bool operator()(const char *key1, const char *key2)const
	{
		return strcmp(key1, key2) < 0;
	}
};

typedef map<const char*, PPER_HANDLE_DATA, cmpkey> SS_MAP_TYPE;

extern SS_MAP_TYPE g_MainLinkMap;
extern CRITICAL_SECTION g_csMainLinkMap;

extern SS_MAP_TYPE g_RedailLinkMap;
extern CRITICAL_SECTION g_csRedailLinkMap;

extern SS_MAP_TYPE g_HBMap;
extern CRITICAL_SECTION g_csHBMap;

extern SS_MAP_TYPE g_ListenMap;
extern CRITICAL_SECTION g_csListenMap;

extern SS_MAP_TYPE g_ProxyMap;
extern CRITICAL_SECTION g_csProxyMap;

extern deque<PPER_HANDLE_DATA> g_PerHandleDq;
extern deque<PPER_IO_DATA> g_PerIoDq;

extern CRITICAL_SECTION g_csPerHandleDq; 
extern CRITICAL_SECTION g_csPerIoDq;

#define SS_SIZE_OF_PER_HANDLE_DATA sizeof(PER_HANDLE_DATA)
#define SS_SIZE_OF_PER_IO_DATA sizeof(PER_IO_DATA)

extern unsigned int g_uiLogThreadId;

extern long g_dwCountOfMLink;
extern CRITICAL_SECTION g_csCountOfMLink;

extern DWORD				g_nSvrListenerCount;
extern PPER_HANDLE_DATA		g_hSvrListener[];
extern HANDLE				g_hSvrListenerEvent[];

extern CRITICAL_SECTION GTcpSvrPendingAcceptCS;

extern PPER_HANDLE_DATA pGTcpSvrPendingAcceptHead;

extern vector<PPER_HANDLE_DATA> AcceptOutT;