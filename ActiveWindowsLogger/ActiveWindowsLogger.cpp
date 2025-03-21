// ActiveWindowsLogger.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "ActiveWindowsLogger.h"
#include <stdio.h>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <Wtsapi32.h>

#include "utils.h"
#include "SystemTray.h"

#include ".\lib\SimpleIni\SimpleIni.h"

#pragma comment( lib, "Wtsapi32.lib" )

namespace fs = std::filesystem;

using namespace std;

HWND hwnd = nullptr;
HPOWERNOTIFY hPowerNotify = nullptr;

HINSTANCE hInst;
LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
HANDLE hNotepadWait = nullptr;

wstring wCurrentWindowName = L"";
wstring wCurrentProcessName = L"";
bool bPaused = false;

std::chrono::high_resolution_clock::time_point iCurrentWindowStartTime = std::chrono::high_resolution_clock::now();
auto lastFlush = std::chrono::high_resolution_clock::now();
auto lastWrite = std::chrono::high_resolution_clock::now();

std::ofstream oFile;

fs::path currentDir = std::filesystem::current_path();
fs::path logsDir = "";
fs::path settingsFile = "";
wstring separator = L",";
wstring fileName = time_stamp(L"%F") + L".csv";

vector<std::wstring> filters;
int iStopLoggingwhenInactiveInterval = 5;

auto currentDay = localtime_xp(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())).tm_mday;
auto lastMove = std::chrono::system_clock::now();
POINT lastCursotPos;

void readSettings() {

	if (!fs::exists(settingsFile)) {
		return;
	}
	CSimpleIniW ini;
	ini.SetUnicode();
	ini.LoadFile(settingsFile.c_str());
	filters = split(ini.GetValue(L"Filters", L"ProgramsFilter", L""), L',');
	separator = ini.GetValue(L"LogsFormat", L"Separator", L",");
	if (separator.size() == 0) {
		separator = L",";
	}
	iStopLoggingwhenInactiveInterval = ini.GetLongValue(L"Tracking", L"StopLoggingwhenInactiveInterval", 5);
}

void write(const chrono::high_resolution_clock::time_point current_time, chrono::milliseconds::rep time) {

	// sometimes, when the PC is waking up after sleep mode, the timer could have value from the beginning of sleeping
	if (time > 62000) {
		time = 62000;
	}

	if (wCurrentProcessName.length() != 0 || wCurrentWindowName.length() != 0) {
		wstringstream line;
		line << time_stamp() << separator;

		replaceAll(wCurrentProcessName, L"\"", L"\"\"");
		line << L"\"" << wCurrentProcessName << L"\"" << separator;

		replaceAll(wCurrentWindowName, L"\"", L"\"\"");
		line << L"\"" << wCurrentWindowName << L"\"" << separator;

		line << time;
		line << endl;

		// if it is the next day, we should start a new file
		auto day = localtime_xp(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())).tm_mday;
		if (day != currentDay) {
			currentDay = day;
			oFile.close();
			fileName = (logsDir / (time_stamp(L"%F") + L".csv")).wstring();
			oFile.open(fileName, std::ios::app);
			string sep_utf8 = to_utf8(separator);
			oFile << "Timestamp" << sep_utf8 << "Program" << sep_utf8 << "WindowTitle" << sep_utf8 << "Time" << endl;
		}

		if (oFile.bad() || !oFile.good() || oFile.fail()) {
			oFile.close();
			oFile.open(fileName, std::ios::app);
		}
		if (!oFile.is_open()) {
			oFile.open(fileName, std::ios::app);
		}

		if (std::find(filters.begin(), filters.end(), wCurrentProcessName) == filters.end()) {
			oFile << to_utf8(line.str());
			lastWrite = current_time;
		}
	}
	// flush the data to the file every 5 seconds
	if (std::chrono::duration_cast<std::chrono::seconds>(current_time - lastFlush).count() > 5) {
		oFile.flush();
		lastFlush = current_time;
	}

}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

	HANDLE hMutexHandle = CreateMutexW(NULL, TRUE, L"ActiveWindowsLogger");
	if (ERROR_ALREADY_EXISTS == GetLastError())
	{
		MessageBox(HWND_DESKTOP, TEXT("This application is already running"), TEXT("Information"), MB_OK | MB_ICONINFORMATION);
		return -1;
	}

	// Elevate some privilegies in the system
	HANDLE hToken;
	// Get a token for this process.
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		// Get the LUID for the shutdown privilege.
		TOKEN_PRIVILEGES tkp = {};
		if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid))
		{
			tkp.PrivilegeCount = 1;  // one privilege to set
			tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			// Get the shutdown privilege for this process.
			AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
		}
		CloseHandle(hToken);
	}

	hInst = hInstance;
	// Register the window class
	WNDCLASSEX window_class_ex = { 0 };
	window_class_ex.cbSize = sizeof(WNDCLASSEX);
	window_class_ex.lpfnWndProc = wnd_proc;
	window_class_ex.lpszClassName = L"Active windows logger";
	window_class_ex.hInstance = hInstance;
	if (!::RegisterClassEx(&window_class_ex)) {
		return 1;
	}

	// Create an overlapped window
	hwnd = ::CreateWindow(
		L"Active windows logger",
		L"",
		WS_OVERLAPPED,
		0, 0, 0, 0,
		nullptr,
		nullptr,
		hInstance,
		0);
	if (!hwnd) {
		return 1;
	}

	const bool isWin8OrLater = IsWindowsVersionOrGreater(6,2);

	// https://learn.microsoft.com/en-us/windows/win32/power/power-setting-guids
	const GUID PowerSettingGuid = isWin8OrLater ? GUID_CONSOLE_DISPLAY_STATE : GUID_MONITOR_POWER_ON;

	hPowerNotify = RegisterPowerSettingNotification((HANDLE)hwnd, &PowerSettingGuid, DEVICE_NOTIFY_WINDOW_HANDLE);

	WTSRegisterSessionNotification(hwnd, NOTIFY_FOR_THIS_SESSION);

	AddNotificationIcon(hwnd, hInstance, bPaused);

	wchar_t processName[2000];
	
	auto AppDataFolder = GetAppDataFolderPath();
	if (AppDataFolder.native().size() > 0) {
		AppDataFolder = AppDataFolder / L"ActiveWindowsLogger";

		if (!fs::exists(AppDataFolder)) {
			fs::create_directory(AppDataFolder);
		}
	}

	if (fs::exists(AppDataFolder)) {
		logsDir = AppDataFolder / "logs";
	}
	else {
		logsDir = currentDir / L"logs";
	}

	if (!fs::exists(logsDir)) {
		if (!fs::create_directory(logsDir)) {
			return -1;
		}
	}
	settingsFile = logsDir / L"settings.ini";
	readSettings();

	fileName = (logsDir / fileName).wstring();
	oFile.open(fileName, std::ios::app);

	if (!oFile.is_open()) {
		return -1;
	}
	if (std::filesystem::file_size(fileName) == 0) {
		string sep_utf8 = to_utf8(separator);
		oFile << "Timestamp" << sep_utf8 << "Program" << sep_utf8 << "WindowTitle" << sep_utf8 << "Time" << endl;
	}
	
	
	int timerID = SetTimer(NULL, 1, 250, NULL);

	MSG msg;

	while (GetMessage(&msg, // message structure 
		NULL,           // handle to window to receive the message 
		0,           // lowest message to examine 
		0))          // highest message to examine 
	{
		
		// if PC is inactive for 5 minutes then we stop register time 
		if (iStopLoggingwhenInactiveInterval > 0) {
			POINT currentCursotPos;
			if (GetCursorPos(&currentCursotPos) && currentCursotPos.x != lastCursotPos.x) {
				lastMove = std::chrono::system_clock::now();
				lastCursotPos = currentCursotPos;
			}
			else if (std::chrono::duration_cast<std::chrono::minutes>(std::chrono::system_clock::now() - lastMove).count() > iStopLoggingwhenInactiveInterval) {
				iCurrentWindowStartTime = std::chrono::high_resolution_clock::now();
				continue;
			}
		}

		if (bPaused) {
			iCurrentWindowStartTime = std::chrono::high_resolution_clock::now();
			continue;
		}

		switch (msg.message)
		{
		case WM_TIMER:
		{
			/*SYSTEM_POWER_STATUS powerStatus;
			if (GetSystemPowerStatus(&powerStatus))
			{
				if (powerStatus.ACLineStatus == 0 && powerStatus.BatteryFlag != 128)
				{
					iCurrentWindowStartTime = std::chrono::high_resolution_clock::now();
					continue;
				}
			}*/

			wstring nextWindowName = L"";
			DWORD processNameLen = 0;

			HWND hActiveWindow = GetForegroundWindow();
			if (hActiveWindow != NULL) {
				int len = GetWindowTextLength(hActiveWindow);
				nextWindowName.clear();
				nextWindowName.resize(len);
				/*int iWindowNameLen = InternalGetWindowText(hActiveWindow, &nextWindowName[0],
					len + 1);*/
				int iWindowNameLen = GetWindowText(hActiveWindow, &nextWindowName[0],
					len + 1);
				if (iWindowNameLen == 0) {
					nextWindowName.clear();
					HWND parendHwnd = GetRealParent(hActiveWindow);
					if (parendHwnd != NULL) {
						len = GetWindowTextLength(parendHwnd);
						nextWindowName.clear();
						nextWindowName.resize(len);
						iWindowNameLen = GetWindowText(parendHwnd, &nextWindowName[0],
							len + 1);
						if (iWindowNameLen == 0) {
							nextWindowName.clear();
						}
					}
				}

				DWORD processID = 0;
				processNameLen = 0;
				GetWindowThreadProcessId(hActiveWindow, &processID);

				if (processID != NULL) {
					HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processID);
					if (hProc == 0) {
						hProc = OpenProcess(MAXIMUM_ALLOWED, FALSE, processID);
					}
					if (hProc != 0) {
						processNameLen = 2000;
						if (!QueryFullProcessImageName(hProc, 0, processName, &processNameLen)) {
							processNameLen = 0;
						};
						CloseHandle(hProc);
					}
				}
			}

			if (wCurrentProcessName.length() > 0 || wCurrentWindowName.length() > 0) {
				
				const auto end_time = std::chrono::high_resolution_clock::now(); // current timestamp
				auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - iCurrentWindowStartTime).count(); // period 

				const auto sinceLastWritePeriod = std::chrono::duration_cast<std::chrono::seconds>(end_time - lastWrite).count();
				if (time > 0 && ((sinceLastWritePeriod > 60) || (nextWindowName.compare(wCurrentWindowName) != 0))) {


					write(end_time, time);

					if (processNameLen > 0) {
						fs::path p(wstring(processName, processNameLen));
						wCurrentProcessName.assign(p.stem().wstring());
					}
					else {
						wCurrentProcessName.clear();
					}

					iCurrentWindowStartTime = end_time;
					wCurrentWindowName.assign(nextWindowName);
					nextWindowName.clear();
				}
			}
			else {
				if (wCurrentWindowName.length() == 0 && nextWindowName.length() > 0) {
					wCurrentWindowName.assign(nextWindowName);
				}
				if (wCurrentProcessName.length() == 0 && processNameLen > 0) {
					fs::path p(wstring(processName, processNameLen));
					wCurrentProcessName.assign(p.stem().wstring());
				}
			}

			break;
		}
			case WM_DESTROY:
			case WM_QUIT:
			case WM_CLOSE:
			{
				// Destroy the timer.

				KillTimer(NULL, 1);
				PostQuitMessage(0);
				break;
			}
		default:
			break;
		}
		TranslateMessage(&msg); // translates virtual-key codes 
		DispatchMessage(&msg);  // dispatches message to window 
	}

	oFile.close();
	ReleaseMutex(hMutexHandle); // Explicitly release mutex
	CloseHandle(hMutexHandle);
	return 0;
	}

// Main window procedure
LRESULT CALLBACK wnd_proc(HWND window_handle, UINT window_message, WPARAM wparam, LPARAM lparam) {
	switch (window_message) {

	case WM_POWERBROADCAST:
	case WM_WTSSESSION_CHANGE:
	case WM_ENDSESSION:
	{
		auto end_time = std::chrono::high_resolution_clock::now();
		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - iCurrentWindowStartTime).count();
		write(end_time, time);
		iCurrentWindowStartTime = end_time;
		break;
	}
	
	case WM_CLOSE:
		DestroyWindow(window_handle);
		break;

	case WM_DESTROY: {
		if (hPowerNotify != NULL) {
			UnregisterPowerSettingNotification(hPowerNotify);
		}		
		WTSUnRegisterSessionNotification(hwnd);
		auto end_time = std::chrono::high_resolution_clock::now();
		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - iCurrentWindowStartTime).count();
		write(end_time, time);
		iCurrentWindowStartTime = end_time;
		DeleteNotificationIcon();
		if (hNotepadWait)
			UnregisterWait(hNotepadWait);
		::PostQuitMessage(0);
		break;
	}
	case WMAPP_NOTIFYCALLBACK:
	{
		int res = 0;
		if (LOWORD(lparam) == NIN_BALLOONUSERCLICK) {
			POINT const pt = { LOWORD(wparam), HIWORD(wparam) };
			res = ShowContextMenu(hwnd, hInst, pt, bPaused);
		}
		else if (LOWORD(lparam) == NIN_SELECT) {
			POINT const pt = { LOWORD(wparam), HIWORD(wparam) };
			res = ShowContextMenu(hwnd, hInst, pt, bPaused);
		}
		else if (LOWORD(lparam) == WM_CONTEXTMENU) {
			POINT const pt = { LOWORD(wparam), HIWORD(wparam) };
			res = ShowContextMenu(hwnd, hInst, pt, bPaused);
		}

		switch (res)
		{

		case IDM_EXIT:
			DestroyWindow(hwnd);
			break;
		case IDM_OPENFOLDER: {
			ShellExecute(NULL, L"open", logsDir.wstring().c_str(), NULL, NULL, SW_SHOWDEFAULT);
			break;
		}
		case IDM_PAUSE: {
			bPaused = true;
			AddNotificationIcon(hwnd, hInst, bPaused, true);
			break;
		}
		case IDM_CONTINUE: {
			bPaused = false;
			AddNotificationIcon(hwnd, hInst, bPaused, true);
			break;
		}
		case IDM_OPENSETTINGS: {
			if (!fs::exists(settingsFile)) {
				CSimpleIniW ini;
				ini.SetUnicode();
				ini.SetValue(L"Filters", L"#ProgramsFilter", L"chrome,foobar2000,firefox", L"#Comma separated string with process names that should be excluded from monitoring");
				ini.SetValue(L"Tracking", L"StopLoggingwhenInactiveInterval", L"5", L"#Suspend monitoring after the interval of inactivity (in minutes). Default - 5 minutes, don't suspend - 0");
				ini.SetValue(L"LogsFormat", L"Separator", L",", L"#CSV columns separator");
				ini.SaveFile(settingsFile.c_str());
			}

			SHELLEXECUTEINFO ShExecInfo = { 0 };
			ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
			ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
			ShExecInfo.hwnd = hwnd;
			ShExecInfo.lpVerb = NULL;
			ShExecInfo.lpFile = settingsFile.c_str();
			ShExecInfo.lpParameters = L"";
			ShExecInfo.lpDirectory = NULL;
			ShExecInfo.nShow = SW_SHOW;
			ShExecInfo.hInstApp = NULL;
			ShellExecuteEx(&ShExecInfo);

			RegisterWaitForSingleObject(&hNotepadWait, ShExecInfo.hProcess,
				[](PVOID lpParameter, BOOLEAN /*TimerOrWaitFired*/)
			{
				CloseHandle(reinterpret_cast<HANDLE>(lpParameter));
				readSettings();
			},
				reinterpret_cast<PVOID>(ShExecInfo.hProcess), INFINITE, WT_EXECUTEONLYONCE);
			break;
		}
		default:
			break;
		}
		break;
	}
	break;
	default:
		return ::DefWindowProc(window_handle, window_message, wparam, lparam);
	}

	return 0;
}