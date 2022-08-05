#include "stdafx.h"
#include "SystemTray.h"
#include "Resource.h"

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment (lib, "comctl32")

#ifdef _DEBUG 
class __declspec(uuid("C805DD46-663F-426C-8C47-95EF8AB5BFE0")) PrinterIcon;
#else 
class __declspec(uuid("87C9AEE8-17BD-408A-A448-3AE7C83AFB22")) PrinterIcon;
#endif

BOOL AddNotificationIcon(HWND hwnd, HINSTANCE hInstance)
{
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.hWnd = hwnd;
	// add the icon, setting the icon, tooltip, and callback message.
	// the icon will be identified with the GUID
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
	nid.guidItem = __uuidof(PrinterIcon);
	nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
	LoadIconMetric(hInstance, MAKEINTRESOURCE(IDI_SMALL), LIM_SMALL, &nid.hIcon);
	//LoadString(hInstance, IDS_TOOLTIP, nid.szTip, ARRAYSIZE(nid.szTip));
	wcscpy_s(nid.szTip, L"\Active windows logger");
	Shell_NotifyIcon(NIM_ADD, &nid);

	// NOTIFYICON_VERSION_4 is prefered
	nid.uVersion = NOTIFYICON_VERSION_4;
	return Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

BOOL DeleteNotificationIcon()
{
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.uFlags = NIF_GUID;
	nid.guidItem = __uuidof(PrinterIcon);
	return Shell_NotifyIcon(NIM_DELETE, &nid);
}

void ShowContextMenu(HWND hwnd, HINSTANCE hInstance, POINT pt)
{
	HMENU hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDC_CONTEXTMENU));
	if (hMenu)
	{
		HMENU hSubMenu = GetSubMenu(hMenu, 0);
		if (hSubMenu)
		{
			// our window must be foreground before calling TrackPopupMenu or the menu will not disappear when the user clicks away
			SetForegroundWindow(hwnd);

			// respect menu drop alignment
			UINT uFlags = TPM_RIGHTBUTTON;
			if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
			{
				uFlags |= TPM_RIGHTALIGN;
			}
			else
			{
				uFlags |= TPM_LEFTALIGN;
			}

			TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL);
		}
		DestroyMenu(hMenu);
	}
}

