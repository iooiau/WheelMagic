/******************************************************************************
*                                                                             *
*    WheelMagic.c                           Copyright(c) 2009-2016 itow,y.    *
*                                                                             *
******************************************************************************/

/******************************************************************************

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

******************************************************************************/


#include "WheelMagic.h"
#include "WheelMagicHook.h"
#include <shellapi.h>
#include "resource.h"


/* ウィンドウクラス名 */
#define WINDOW_CLASS_NAME APP_NAME TEXT(" Window")
/* トレイアイコン用のメッセージ */
#define WM_APP_TRAYICON	WM_APP
/* トレイアイコン登録タイマ識別子 */
#define TIMER_TRAYICON	1
/* トレイアイコン登録のリトライ間隔 */
#define TRAY_ICON_RETRY_INTERVAL	5000
/* トレイアイコン登録の最大リトライ回数 */
#define MAX_TRAY_ICON_RETRY	12


/* インスタンスハンドル */
#ifdef HOOK_DLL
static
#endif
HINSTANCE hInst;


/* メッセージ表示 */
static int ShowMessage(int ID, UINT Type)
{
	TCHAR szText[80];

	if (LoadString(hInst, ID, szText, sizeof(szText) / sizeof(TCHAR)) == 0)
		return 0;
	return MessageBox(NULL, szText, APP_NAME, Type);
}


/* エラーメッセージ表示 */
static void ShowErrorMessage(int ID)
{
	ShowMessage(ID, MB_OK | MB_ICONSTOP);
}


/* タスクトレイにアイコンを追加 */
static BOOL AddTrayIcon(HWND hwnd, BOOL fModify)
{
	NOTIFYICONDATA nid;

#if NTDDI_VERSION >= NTDDI_WINXP
	nid.cbSize = NOTIFYICONDATA_V2_SIZE;
#else
	nid.cbSize = sizeof(NOTIFYICONDATA);
#endif
	nid.hWnd = hwnd;
	nid.uID = 1;
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	nid.uCallbackMessage = WM_APP_TRAYICON;
	nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON));
	lstrcpy(nid.szTip, APP_NAME);
	return Shell_NotifyIcon(fModify ? NIM_MODIFY : NIM_ADD, &nid);
}


/* タスクトレイからアイコンを削除 */
static BOOL RemoveTrayIcon(HWND hwnd)
{
	NOTIFYICONDATA nid;

#if NTDDI_VERSION >= NTDDI_WINXP
	nid.cbSize = NOTIFYICONDATA_V2_SIZE;
#else
	nid.cbSize = sizeof(NOTIFYICONDATA);
#endif
	nid.hWnd = hwnd;
	nid.uID = 1;
	nid.uFlags = 0;
	return Shell_NotifyIcon(NIM_DELETE, &nid);
}


/* ウィンドウプロシージャ */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	/* "TaskbarCreated" メッセージ識別子 */
	static UINT TaskbarCreatedMessage = 0;
	/* タスクトレイアイコン登録リトライカウンタ */
	static int TrayIconRetryCount = 0;

	switch (uMsg) {
	case WM_CREATE:
		{
			/* タスクバー作成メッセージの取得 */
			TaskbarCreatedMessage = RegisterWindowMessage(TEXT("TaskbarCreated"));

			/* Vista/7 で管理者権限で実行された場合のためにメッセージの受信を許可 */
			{
#ifndef MSGFLT_ADD
#define MSGFLT_ADD 1
#endif
				typedef BOOL (WINAPI *ChangeWindowMessageFilterFunc)(UINT, DWORD);
				HMODULE hLib = GetModuleHandle(TEXT("user32.dll"));
				if (hLib != NULL) {
					ChangeWindowMessageFilterFunc pChangeFilter =
						(ChangeWindowMessageFilterFunc)
							GetProcAddress(hLib, "ChangeWindowMessageFilter");
					if (pChangeFilter != NULL) {
						pChangeFilter(TaskbarCreatedMessage, MSGFLT_ADD);
						pChangeFilter(WM_APP_TRAYICON, MSGFLT_ADD);
					}
				}
			}

			/* タスクトレイにアイコンを追加 */
			if (!AddTrayIcon(hwnd, FALSE)) {
				/*
					リトライするためにタイマー生成
					(Windows 起動直後など、ビジー状態でトレイへのアイコン登録が失敗することがある)
				*/
				SetTimer(hwnd, TIMER_TRAYICON, TRAY_ICON_RETRY_INTERVAL, NULL);
			}
		}
		return 0;

	case WM_TIMER:
		/* トレイアイコン追加のリトライ */
		if (wParam == TIMER_TRAYICON) {
			if (AddTrayIcon(hwnd, TRUE) || AddTrayIcon(hwnd, FALSE)) {
				KillTimer(hwnd, TIMER_TRAYICON);
			} else if (++TrayIconRetryCount >= MAX_TRAY_ICON_RETRY) {
				KillTimer(hwnd, TIMER_TRAYICON);
				ShowErrorMessage(IDS_ERR_ADDTRAYICON);
				DestroyWindow(hwnd);
			}
		}
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case CM_EXIT:
			/* プログラム終了 */
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			return 0;
		}
		return 0;

	case WM_APP_TRAYICON:
		/* トレイアイコンのメッセージ */
		if (lParam == WM_RBUTTONUP) {
			HMENU hmenu = LoadMenu(hInst, MAKEINTRESOURCE(IDM_TRAY));
			POINT pt;

			SetForegroundWindow(hwnd);
			GetCursorPos(&pt);
			TrackPopupMenu(GetSubMenu(hmenu, 0), TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
			DestroyMenu(hmenu);
			/*
				WM_NULL のポストが必要な理由は以下を参照
				https://support.microsoft.com/en-us/kb/135788
			*/
			PostMessage(hwnd, WM_NULL, 0, 0);
		} else if (lParam == WM_LBUTTONDBLCLK) {
			SendMessage(hwnd, WM_CLOSE, 0, 0);
		}
		return 0;

	case WM_DESTROY:
		/* 終了処理 */
		RemoveTrayIcon(hwnd);
		PostQuitMessage(0);
		return 0;

	default:
		/* タスクバーの再作成時にアイコンを登録し直す */
		if (TaskbarCreatedMessage != 0 && uMsg == TaskbarCreatedMessage) {
			if (!AddTrayIcon(hwnd, FALSE)) {
				/* リトライするためにタイマー生成 */
				TrayIconRetryCount = 0;
				SetTimer(hwnd, TIMER_TRAYICON, TRAY_ICON_RETRY_INTERVAL, NULL);
			}
			return 0;
		}
		break;
	}

	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}


static BOOL AppMain(void)
{
	HWND hwnd;
#ifdef HOOK_DLL
	HMODULE hHookLib;
	WMBeginHookFunc pBeginHook;
	WMEndHookFunc pEndHook;
#endif

	/* ウィンドウクラスの登録 */
	{
		WNDCLASS wc;
		wc.style = 0;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInst;
		wc.hIcon = NULL;
		wc.hCursor = NULL;
		wc.hbrBackground = NULL;
		wc.lpszMenuName = NULL;
		wc.lpszClassName = WINDOW_CLASS_NAME;
		if (RegisterClass(&wc) == 0) {
			ShowErrorMessage(IDS_ERR_REGISTERCLASS);
			return FALSE;
		}
	}

	/* ウィンドウの作成 */
	hwnd = CreateWindowEx(0, WINDOW_CLASS_NAME, APP_NAME, WS_POPUP,
						  0, 0, 0, 0, NULL, NULL, hInst, NULL);
	if (hwnd == NULL) {
		ShowErrorMessage(IDS_ERR_CREATEWINDOW);
		return FALSE;
	}

	/* フックの設定 */
#ifdef HOOK_DLL
	{
		TCHAR szHookDllPath[MAX_PATH];
		LPTSTR pszFileName, p;

		GetModuleFileName(NULL, szHookDllPath, sizeof(szHookDllPath) / sizeof(TCHAR));
		pszFileName = szHookDllPath;
		for (p = szHookDllPath; *p != TEXT('\0'); p++) {
			if (*p == TEXT('\\'))
				pszFileName = p + 1;
#ifndef UNICODE
			else if (IsDBCSLeadByteEx(CP_ACP, *p))
				p++;
#endif
		}
		::lstrcpy(pszFileName,
#ifndef WIN64
			TEXT("WheelMagicHook.dll")
#else
			TEXT("WheelMagicHook64.dll")
#endif
		);
		hHookLib = LoadLibrary(szHookDllPath);
		if (hHookLib == NULL) {
			ShowErrorMessage(IDS_ERR_HOOKDLLLOAD);
			DestroyWindow(hwnd);
			return FALSE;
		}
		pBeginHook = (WMBeginHookFunc)GetProcAddress(hHookLib, "WMBeginHook");
		pEndHook = (WMEndHookFunc)GetProcAddress(hHookLib, "WMEndHook");
		if (pBeginHook == NULL || pEndHook == NULL) {
			FreeLibrary(hHookLib);
			ShowErrorMessage(IDS_ERR_PROCADDRESS);
			DestroyWindow(hwnd);
			return FALSE;
		}
		if (!(*pBeginHook)()) {
			FreeLibrary(hHookLib);
			ShowErrorMessage(IDS_ERR_BEGINHOOK);
			DestroyWindow(hwnd);
			return FALSE;
		}
	}
#else	/* HOOK_DLL */
	if (!WMBeginHook()) {
		ShowErrorMessage(IDS_ERR_BEGINHOOK);
		DestroyWindow(hwnd);
		return FALSE;
	}
#endif	/* ndef HOOK_DLL */

	/* メッセージループ */
	{
		MSG msg;
		BOOL Result;

		while ((Result = GetMessage(&msg, NULL, 0, 0)) != 0) {
			if (Result > 0) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			} else {
				DestroyWindow(hwnd);
				ShowErrorMessage(IDS_ERR_GETMESSAGE);
				break;
			}
		}
	}

	/* フックの解除 */
#ifdef HOOK_DLL
	(*pEndHook)();
	FreeLibrary(hHookLib);
#else
	WMEndHook();
#endif

	return TRUE;
}


/* メイン関数 */
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
					   LPTSTR pszCmdLine, int nCmdShow)
{
	HANDLE hMutex;

	/* 複数起動のチェック */
	hMutex = CreateMutex(NULL, TRUE, APP_NAME TEXT(" Mutex"));
	if (hMutex == NULL) {
		ShowErrorMessage(IDS_ERR_CREATEMUTEX);
		return 0;
	}
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		/* 既に起動している */
		ShowMessage(IDS_ERR_ALREADYRUNNING, MB_OK | MB_ICONINFORMATION);
		return 0;
	}

	hInst = hInstance;

	AppMain();

	ReleaseMutex(hMutex);
	CloseHandle(hMutex);

	return 0;
}


#ifndef _DEBUG

/* プログラムサイズを小さくするためにCRTを除外 */
#pragma comment(linker, "/nodefaultlib:libcmt.lib")
#pragma comment(linker, "/entry:Startup")

/* エントリポイント */
void WINAPI Startup(void)
{
	_tWinMain(GetModuleHandle(NULL), NULL, TEXT(""), SW_SHOWNORMAL);
	ExitProcess(0);
}

#endif
