/******************************************************************************
*                                                                             *
*    WheelMagicHook.c                       Copyright(c) 2009-2016 itow,y.    *
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


#ifdef HOOK_DLL


#ifndef _DEBUG
/* プログラムサイズを小さくするためにCRTを除外 */
#pragma comment(linker, "/nodefaultlib:libcmt.lib")
#pragma comment(linker, "/entry:DllMain")
#endif

#define EXPORT(type) __declspec(dllexport) type WINAPI

#pragma data_seg(".SHARE")
/* フックハンドル */
HHOOK hHook=NULL;
#pragma data_seg()

/* インスタンスハンドル */
static HINSTANCE hInst;


/* エントリポイント */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pvReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH) {
		hInst = hInstance;
	}
	return TRUE;
}


#else	/* HOOK_DLLL */


#define EXPORT(type) type

/* フックハンドル */
static HHOOK hHook = NULL;
/* インスタンスハンドル */
extern HINSTANCE hInst;


#endif	/* ndef HOOK_DLL */


/* 位置からウィンドウを探すための情報 */
typedef struct {
	POINT ptPos;
	HWND hwnd;
} FindWindowInfo;


/* 位置からウィンドウを探すためのコールバック関数 */
static BOOL CALLBACK FindWindowProc(HWND hwnd, LPARAM lParam)
{
	FindWindowInfo *pInfo = (FindWindowInfo*)lParam;
	RECT rc;
	TCHAR szClass[64];

	if (hwnd != pInfo->hwnd
			&& IsWindowVisible(hwnd)
			&& GetWindowRect(hwnd, &rc)
			&& PtInRect(&rc, pInfo->ptPos)
			&& GetClassName(hwnd, szClass, sizeof(szClass) / sizeof(TCHAR)) > 0
			&& lstrcmpi(szClass, TEXT("tooltips_class32")) != 0
			&& lstrcmpi(szClass, TEXT("SysShadow")) != 0) {
		pInfo->hwnd = hwnd;
		return FALSE;
	}
	return TRUE;
}


/* フックプロシージャ */
static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif
	if (nCode == HC_ACTION
			&& (wParam == WM_MOUSEWHEEL || wParam == WM_MOUSEHWHEEL)) {
		MSLLHOOKSTRUCT *pmhs = (MSLLHOOKSTRUCT*)lParam;
		POINT ptCursor = pmhs->pt;
		HWND hwnd = WindowFromPoint(ptCursor);

		if (hwnd != NULL) {
			HWND hwndTarget;
			TCHAR szClass[64];
			POINT ptClient;

			if (GetClassName(hwnd, szClass, sizeof(szClass) / sizeof(TCHAR)) > 0
					&& lstrcmpi(szClass, TEXT("tooltips_class32")) == 0) {
				/* Tooltipに重なる場合、下のウィンドウを探す */
				FindWindowInfo Info;

				Info.ptPos = ptCursor;
				Info.hwnd = hwnd;
				EnumWindows(FindWindowProc, (LPARAM)&Info);
				if (Info.hwnd == hwnd)
					goto Skip;
				hwndTarget = Info.hwnd;
				hwnd = Info.hwnd;
				for (;;) {
					ptClient = ptCursor;
					ScreenToClient(hwnd, &ptClient);
					hwnd = RealChildWindowFromPoint(hwnd, ptClient);
					if (hwnd == NULL || hwnd == hwndTarget)
						break;
					hwndTarget = hwnd;
				}
			} else {
				hwndTarget = hwnd;
				if ((GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD) != 0) {
					HWND hwndParent = GetParent(hwnd);

					if (hwndParent != NULL) {
						ptClient = ptCursor;
						ScreenToClient(hwndParent, &ptClient);
						hwndTarget = RealChildWindowFromPoint(hwndParent, ptClient);
						if (hwndTarget == NULL)
							hwndTarget = hwnd;
					}
				}
			}
			{
				WORD KeyState = 0;
				if (GetAsyncKeyState(VK_LBUTTON) < 0)
					KeyState |= MK_LBUTTON;
				if (GetAsyncKeyState(VK_RBUTTON) < 0)
					KeyState |= MK_RBUTTON;
				if (GetAsyncKeyState(VK_MBUTTON) < 0)
					KeyState |= MK_MBUTTON;
				if (GetAsyncKeyState(VK_XBUTTON1) < 0)
					KeyState |= MK_XBUTTON1;
				if (GetAsyncKeyState(VK_XBUTTON2) < 0)
					KeyState |= MK_XBUTTON2;
				if (GetAsyncKeyState(VK_SHIFT) < 0)
					KeyState |= MK_SHIFT;
				if (GetAsyncKeyState(VK_CONTROL) < 0)
					KeyState |= MK_CONTROL;
#ifdef _DEBUG
				{
					TCHAR szLog[256];
					GetClassName(hwndTarget, szClass, sizeof(szClass) / sizeof(TCHAR));
					wsprintf(szLog, TEXT("%ld, %ld : %p \"%s\" %u\n"),
						ptCursor.x, ptCursor.y, hwndTarget, szClass, (UINT)wParam);
					OutputDebugString(szLog);
				}
#endif
				PostMessage(hwndTarget, (UINT)wParam,
							MAKEWPARAM(KeyState, HIWORD(pmhs->mouseData)),
							MAKELPARAM((SHORT)ptCursor.x, (SHORT)ptCursor.y));
				return 1;
			}
		}
	}

Skip:
	return CallNextHookEx(hHook, nCode, wParam, lParam);
}


/* フックの開始 */
EXPORT(BOOL) WMBeginHook(void)
{
	if (hHook == NULL) {
		hHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, hInst, 0);
		if (hHook == NULL)
			return FALSE;
	}
	return TRUE;
}


/* フックの終了 */
EXPORT(BOOL) WMEndHook(void)
{
	if (hHook != NULL) {
		UnhookWindowsHookEx(hHook);
		hHook = NULL;
	}
	return TRUE;
}
