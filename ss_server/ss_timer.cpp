#include "ss_timer.h"
#include "tool.h"

unsigned int _stdcall ss_timer(LPVOID pVoid)
{
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	UINT nTimerId01 = SetTimer(NULL, 1, 1000*5, ss_hb_timer);
	UINT nTimerId02 = SetTimer(NULL, 2, 1000*10, ss_proxy_timer);
	UINT nTimerId03 = SetTimer(NULL, 3, 1000*20, ss_listen_timer);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (msg.message == WM_TIMER)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	KillTimer(NULL, nTimerId01);
	KillTimer(NULL, nTimerId02);
	KillTimer(NULL, nTimerId03);

	return 0;
}