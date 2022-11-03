#pragma once

#include <shellapi.h>
#include <commctrl.h>

UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;
UINT const WMAPP_HIDEFLYOUT = WM_APP + 2;

BOOL AddNotificationIcon(HWND hwnd, HINSTANCE hInstance, bool bPaused, bool bUpdate = false);
int ShowContextMenu(HWND hwnd, HINSTANCE hInstance, POINT pt, bool bPaused);
BOOL DeleteNotificationIcon();
BOOL ShowLowInkBalloon(HINSTANCE hInstance);
