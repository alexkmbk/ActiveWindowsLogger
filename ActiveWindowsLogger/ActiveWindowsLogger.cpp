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

#pragma comment( lib, "Wtsapi32.lib" )

//#include "stxutif.h"

namespace fs = std::filesystem;

using namespace std;

HWND hwnd = nullptr;
HINSTANCE hInst;
LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);

wstring wCurrentWindowName = L"";
wstring wCurrentProcessName = L"";

std::chrono::high_resolution_clock::time_point iCurrentWindowStartTime = std::chrono::high_resolution_clock::now();
auto lastFlush = std::chrono::high_resolution_clock::now();
auto lastWrite = std::chrono::high_resolution_clock::now();

std::ofstream oFile;

fs::path currentDir = std::filesystem::current_path();
fs::path logsDir = "";
wstring fileName = time_stamp(L"%F") + L".csv";

vector<std::wstring> filters;

auto currentDay = localtime_xp(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())).tm_mday;
auto lastMove = std::chrono::system_clock::now();
POINT lastCursotPos;

void write(chrono::high_resolution_clock::time_point current_time, chrono::milliseconds::rep time) {

	if (wCurrentProcessName.length() != 0 || wCurrentWindowName.length() != 0) {
		wstringstream line;
		line << time_stamp() << ",";

		replaceAll(wCurrentProcessName, L"\"", L"\"\"");
		line << L"\"" << wCurrentProcessName << L"\",";

		replaceAll(wCurrentWindowName, L"\"", L"\"\"");
		line << L"\"" << wCurrentWindowName << L"\",";

		line << time;
		line << endl;

		// if it is the next day, we should start a new file
		auto day = localtime_xp(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())).tm_mday;
		if (day != currentDay) {
			currentDay = day;
			oFile.close();
			fileName = (logsDir / (time_stamp(L"%F") + L".csv")).wstring();
			oFile.open(fileName, std::ios::app);
			oFile << "Timestamp," << "Program," << "WindowTitle," << "time" << endl;
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

	if (!CheckUniqueProcess())
	{
		MessageBox(HWND_DESKTOP, TEXT("This application is already running"), TEXT("Information"), MB_OK | MB_ICONINFORMATION);
		return -1;
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

	WTSRegisterSessionNotification(hwnd, NOTIFY_FOR_THIS_SESSION);

	AddNotificationIcon(hwnd, hInstance);
	//ShowLowInkBalloon(hInstance);


	wchar_t processName[2000];
	
	//std::locale utf8_locale(std::locale(), new utf8cvt<false>);
	//std::wofstream.imbue(utf8_locale);

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

	// load filters

	std::wifstream rfile;

	fs::path filterFilename = logsDir / L"filter.txt";

	if (!fs::exists(filterFilename)) {
		filterFilename = currentDir / L"filter.txt";
	}

	rfile.open(filterFilename);
	if (rfile.is_open()) {
		std::wstring line;
		std::getline(rfile, line);
		filters = split(line, L',');
		rfile.close();
	}

	fileName = (logsDir / fileName).wstring();

	//oFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	
	oFile.open(fileName, std::ios::app);

	if (!oFile.is_open()) {
		//MessageBoxA(0, strerror(errno), "ActiveWindowsLogger", MB_OK | MB_ICONINFORMATION);
		return -1;
	}
	if (std::filesystem::file_size(fileName) == 0) {
		oFile << "Timestamp," << "Program," << "WindowTitle," << "time" << endl;
	}
	
	
	int timerID = SetTimer(NULL, 1, 250, NULL);

	MSG msg;

	while (GetMessage(&msg, // message structure 
		NULL,           // handle to window to receive the message 
		0,           // lowest message to examine 
		0))          // highest message to examine 
	{
		
		// if PC is inactive for 5 minutes then we stop register time 
		POINT currentCursotPos;
		if (GetCursorPos(&currentCursotPos) && currentCursotPos.x != lastCursotPos.x) {
			lastMove = std::chrono::system_clock::now();
			lastCursotPos = currentCursotPos;
		}
		else if (std::chrono::duration_cast<std::chrono::minutes>(std::chrono::system_clock::now() - lastMove).count() > 5) {
			continue;
		}		
		
		// Post WM_TIMER messages to the hwndTimer procedure. 


		//if (msg.message == WM_TIMER) {
		switch (msg.message)
		{
		case WM_TIMER:
		{
			//
			// Process other messages. 
			//
			wstring nextWindowName = L"";
			DWORD processNameLen = 0;

			HWND hActiveWindow = GetForegroundWindow();
			if (hActiveWindow != NULL) {
				int len = GetWindowTextLength(hActiveWindow);
				nextWindowName.clear();
				nextWindowName.resize(len);
				int iWindowNameLen = GetWindowText(hActiveWindow, &nextWindowName[0],
					len + 1);
				if (iWindowNameLen == 0) {
					int error = GetLastError();
					nextWindowName.clear();
				}

				DWORD processID = 0;
				processNameLen = 0;
				GetWindowThreadProcessId(hActiveWindow, &processID);

				if (processID != NULL) {
					HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
					if (hProc != 0) {
						processNameLen = 2000;
						if (!QueryFullProcessImageName(hProc, 0, processName, &processNameLen)) {
							processNameLen = 0;
						};
					}
				}
			}

			if (wCurrentProcessName.length() > 0 || wCurrentWindowName.length() > 0) {

				auto end_time = std::chrono::high_resolution_clock::now();
				auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - iCurrentWindowStartTime).count();

				if (time > 0 && (std::chrono::duration_cast<std::chrono::seconds>(end_time - lastWrite).count() > 60) || (nextWindowName.compare(wCurrentWindowName) != 0)) {

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
	return 0;
	}

// Main window procedure
LRESULT CALLBACK wnd_proc(HWND window_handle, UINT window_message, WPARAM wparam, LPARAM lparam) {
	switch (window_message) {

	case WM_POWERBROADCAST:
	{
		auto end_time = std::chrono::high_resolution_clock::now();
		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - iCurrentWindowStartTime).count();
		write(end_time, time);
		iCurrentWindowStartTime = end_time;
		break;
	}
	case WM_WTSSESSION_CHANGE:
	{
		auto end_time = std::chrono::high_resolution_clock::now();
		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - iCurrentWindowStartTime).count();
		write(end_time, time);
		iCurrentWindowStartTime = end_time;
		break;
	}
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
		WTSUnRegisterSessionNotification(hwnd);
		auto end_time = std::chrono::high_resolution_clock::now();
		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - iCurrentWindowStartTime).count();
		write(end_time, time);
		DeleteNotificationIcon();
		::PostQuitMessage(0);
		break;
	}
	case WMAPP_NOTIFYCALLBACK:
		if (LOWORD(lparam) == NIN_BALLOONUSERCLICK) {
			POINT const pt = { LOWORD(wparam), HIWORD(wparam) };
			ShowContextMenu(hwnd, hInst, pt);
		} else if (LOWORD(lparam) == NIN_SELECT) {
			POINT const pt = { LOWORD(wparam), HIWORD(wparam) };
			ShowContextMenu(hwnd, hInst, pt);
		}
		else if (LOWORD(lparam) == WM_CONTEXTMENU) {
			POINT const pt = { LOWORD(wparam), HIWORD(wparam) };
			ShowContextMenu(hwnd, hInst, pt);
		}
		break;

	case WM_COMMAND:
	{
		int const wmId = LOWORD(wparam);
		// Parse the menu selections:
		switch (wmId)
		{

		case IDM_EXIT:
			DestroyWindow(hwnd);
			break;
		case IDM_OPENFOLDER:
			ShellExecute(NULL, L"open", logsDir.wstring().c_str(), NULL, NULL, SW_SHOWDEFAULT);
			break;
		default:
			return DefWindowProc(hwnd, window_message, wparam, lparam);
		}
	}
	break;
	default:
		return ::DefWindowProc(window_handle, window_message, wparam, lparam);
	}

	return 0;
}