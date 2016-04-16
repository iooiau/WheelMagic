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


/* �E�B���h�E�N���X�� */
#define WINDOW_CLASS_NAME APP_NAME TEXT(" Window")
/* �g���C�A�C�R���p�̃��b�Z�[�W */
#define WM_APP_TRAYICON	WM_APP
/* �g���C�A�C�R���o�^�^�C�}���ʎq */
#define TIMER_TRAYICON	1
/* �g���C�A�C�R���o�^�̃��g���C�Ԋu */
#define TRAY_ICON_RETRY_INTERVAL	5000
/* �g���C�A�C�R���o�^�̍ő僊�g���C�� */
#define MAX_TRAY_ICON_RETRY	12


/* �C���X�^���X�n���h�� */
#ifdef HOOK_DLL
static
#endif
HINSTANCE hInst;


/* ���b�Z�[�W�\�� */
static int ShowMessage(int ID, UINT Type)
{
	TCHAR szText[80];

	if (LoadString(hInst, ID, szText, sizeof(szText) / sizeof(TCHAR)) == 0)
		return 0;
	return MessageBox(NULL, szText, APP_NAME, Type);
}


/* �G���[���b�Z�[�W�\�� */
static void ShowErrorMessage(int ID)
{
	ShowMessage(ID, MB_OK | MB_ICONSTOP);
}


/* �^�X�N�g���C�ɃA�C�R����ǉ� */
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


/* �^�X�N�g���C����A�C�R�����폜 */
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


/* �E�B���h�E�v���V�[�W�� */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	/* "TaskbarCreated" ���b�Z�[�W���ʎq */
	static UINT TaskbarCreatedMessage = 0;
	/* �^�X�N�g���C�A�C�R���o�^���g���C�J�E���^ */
	static int TrayIconRetryCount = 0;

	switch (uMsg) {
	case WM_CREATE:
		{
			/* �^�X�N�o�[�쐬���b�Z�[�W�̎擾 */
			TaskbarCreatedMessage = RegisterWindowMessage(TEXT("TaskbarCreated"));

			/* Vista/7 �ŊǗ��Ҍ����Ŏ��s���ꂽ�ꍇ�̂��߂Ƀ��b�Z�[�W�̎�M������ */
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

			/* �^�X�N�g���C�ɃA�C�R����ǉ� */
			if (!AddTrayIcon(hwnd, FALSE)) {
				/*
					���g���C���邽�߂Ƀ^�C�}�[����
					(Windows �N������ȂǁA�r�W�[��ԂŃg���C�ւ̃A�C�R���o�^�����s���邱�Ƃ�����)
				*/
				SetTimer(hwnd, TIMER_TRAYICON, TRAY_ICON_RETRY_INTERVAL, NULL);
			}
		}
		return 0;

	case WM_TIMER:
		/* �g���C�A�C�R���ǉ��̃��g���C */
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
			/* �v���O�����I�� */
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			return 0;
		}
		return 0;

	case WM_APP_TRAYICON:
		/* �g���C�A�C�R���̃��b�Z�[�W */
		if (lParam == WM_RBUTTONUP) {
			HMENU hmenu = LoadMenu(hInst, MAKEINTRESOURCE(IDM_TRAY));
			POINT pt;

			SetForegroundWindow(hwnd);
			GetCursorPos(&pt);
			TrackPopupMenu(GetSubMenu(hmenu, 0), TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
			DestroyMenu(hmenu);
			/*
				WM_NULL �̃|�X�g���K�v�ȗ��R�͈ȉ����Q��
				https://support.microsoft.com/en-us/kb/135788
			*/
			PostMessage(hwnd, WM_NULL, 0, 0);
		} else if (lParam == WM_LBUTTONDBLCLK) {
			SendMessage(hwnd, WM_CLOSE, 0, 0);
		}
		return 0;

	case WM_DESTROY:
		/* �I������ */
		RemoveTrayIcon(hwnd);
		PostQuitMessage(0);
		return 0;

	default:
		/* �^�X�N�o�[�̍č쐬���ɃA�C�R����o�^������ */
		if (TaskbarCreatedMessage != 0 && uMsg == TaskbarCreatedMessage) {
			if (!AddTrayIcon(hwnd, FALSE)) {
				/* ���g���C���邽�߂Ƀ^�C�}�[���� */
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

	/* �E�B���h�E�N���X�̓o�^ */
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

	/* �E�B���h�E�̍쐬 */
	hwnd = CreateWindowEx(0, WINDOW_CLASS_NAME, APP_NAME, WS_POPUP,
						  0, 0, 0, 0, NULL, NULL, hInst, NULL);
	if (hwnd == NULL) {
		ShowErrorMessage(IDS_ERR_CREATEWINDOW);
		return FALSE;
	}

	/* �t�b�N�̐ݒ� */
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

	/* ���b�Z�[�W���[�v */
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

	/* �t�b�N�̉��� */
#ifdef HOOK_DLL
	(*pEndHook)();
	FreeLibrary(hHookLib);
#else
	WMEndHook();
#endif

	return TRUE;
}


/* ���C���֐� */
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
					   LPTSTR pszCmdLine, int nCmdShow)
{
	HANDLE hMutex;

	/* �����N���̃`�F�b�N */
	hMutex = CreateMutex(NULL, TRUE, APP_NAME TEXT(" Mutex"));
	if (hMutex == NULL) {
		ShowErrorMessage(IDS_ERR_CREATEMUTEX);
		return 0;
	}
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		/* ���ɋN�����Ă��� */
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

/* �v���O�����T�C�Y�����������邽�߂�CRT�����O */
#pragma comment(linker, "/nodefaultlib:libcmt.lib")
#pragma comment(linker, "/entry:Startup")

/* �G���g���|�C���g */
void WINAPI Startup(void)
{
	_tWinMain(GetModuleHandle(NULL), NULL, TEXT(""), SW_SHOWNORMAL);
	ExitProcess(0);
}

#endif
