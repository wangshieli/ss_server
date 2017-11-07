#include "ss_log_show.h"

#define SIZE_LOG_NAME			128

unsigned int _stdcall ss_log_show(LPVOID pVoid)
{
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	DWORD FileAttr;
	FILE *fp;
	char szLogFileName[SIZE_LOG_NAME];
	char szCurrentTime[SIZE_LOG_NAME];
	if ((FileAttr = GetFileAttributesA("LOG")) == 0xffffffff)
		CreateDirectoryA("LOG", NULL);

	SYSTEMTIME CurSysTime;
	GetSystemTime(&CurSysTime);

	sprintf_s(szLogFileName, "LOG\\%04d-%02d-%02d.LOG",
		CurSysTime.wYear,
		CurSysTime.wMonth,
		CurSysTime.wDay);
	fopen_s(&fp, szLogFileName, "a+b");
	if (NULL != fp)
	{
		while (GetMessage(&msg, NULL, 0, 0))
		{
			switch (msg.message)
			{
			case SS_LOG_SHOW:
				{
					char* pLog = (char*)msg.wParam;
					SYSTEMTIME sys;
					GetLocalTime(&sys);
					printf("%d-%d-%d %02d:%02d:%02d:%s\n", sys.wYear, 
						sys.wMonth, 
						sys.wDay, 
						sys.wHour, 
						sys.wMinute, 
						sys.wSecond,
						pLog);
					delete pLog;
					pLog = NULL;
				}
				break;

			case SS_LOG_TEXT:
				{
					char* pLog = (char*)msg.wParam;
					SYSTEMTIME MsgSysTime;
					GetSystemTime(&MsgSysTime);
					if (CurSysTime.wDay != MsgSysTime.wDay)
					{
						fclose(fp);
						CurSysTime = MsgSysTime;
						sprintf_s(szLogFileName, "LOG\\%04d-%02d-%02d.LOG",
							CurSysTime.wYear,
							CurSysTime.wMonth,
							CurSysTime.wDay);
						fopen_s(&fp, szLogFileName, "a+b");
					}
					sprintf_s(szCurrentTime, "%04d-%02d-%02d %02d:%02d:%02d\t",
						MsgSysTime.wYear,
						MsgSysTime.wMonth,
						MsgSysTime.wDay,
						MsgSysTime.wHour,
						MsgSysTime.wMinute,
						MsgSysTime.wSecond);
					fputs(szCurrentTime, fp);
					fputs(pLog, fp);
					fputs("\r\n", fp);
					fflush(fp);
					delete pLog;
					pLog = NULL;
				}
				break;
			default:
				{
					printf("日志记录出错\n");
				}
				break;
			}
		}
		fclose(fp);
	}else
	{
		printf("创建日志文件失败\n");
	}
	return 0;
}