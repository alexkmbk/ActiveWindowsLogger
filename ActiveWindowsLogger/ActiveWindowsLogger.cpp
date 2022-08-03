// ActiveWindowsLogger.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "ActiveWindowsLogger.h"
#include <stdio.h>
#include <string>
#include <fstream>
#include <filesystem>
#include "utils.h"
#include <sstream>
//#include "stxutif.h"

namespace fs = std::filesystem;

using namespace std;

wstring wCurrentWindowName = L"";
wstring wCurrentProcessName = L"";

std::chrono::high_resolution_clock::time_point iCurrentWindowStartTime = std::chrono::high_resolution_clock::now();
auto lastFlush = std::chrono::high_resolution_clock::now();

std::wofstream oFile;
wstring fileName = L"logs_" + time_stamp(L"%F") + L".csv";

auto currentDay = localtime_xp(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())).tm_mday;

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

	HANDLE hMutex = CreateMutexA(NULL, FALSE, "my mutex");
	DWORD dwMutexWaitResult = WaitForSingleObject(hMutex, 0);
	if (dwMutexWaitResult != WAIT_OBJECT_0)
	{
		MessageBox(HWND_DESKTOP, TEXT("This application is already running"), TEXT("Information"), MB_OK | MB_ICONINFORMATION);
		CloseHandle(hMutex);
		return -1;
	}

	fs::path currentDir = std::filesystem::current_path();
	fs::path logsDir = currentDir / L"logs";

	wchar_t processName[2000];
	
	//std::locale utf8_locale(std::locale(), new utf8cvt<false>);
	//std::wofstream.imbue(utf8_locale);

	fileName = (logsDir / fileName).wstring();

		if (!fs::exists(logsDir)) {
			if (!fs::create_directory(logsDir)) {
				return -1;
			}
	}
	//oFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	
	oFile.open(fileName, std::ios::app);

	if (!oFile.is_open()) {
		//MessageBoxA(0, strerror(errno), "ActiveWindowsLogger", MB_OK | MB_ICONINFORMATION);
		return -1;
	}
	if (std::filesystem::file_size(fileName) == 0) {
		oFile << L"Timestamp," << "Program," << "WindowTitle," << "time" << endl;
	}
	
	
	int timerID = SetTimer(NULL, 1, 250, NULL);

	MSG msg;

	while (GetMessage(&msg, // message structure 
		NULL,           // handle to window to receive the message 
		0,           // lowest message to examine 
		0))          // highest message to examine 
	{

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

				bool WindowNameChanged = (nextWindowName.compare(wCurrentWindowName) != 0);
				
				if (WindowNameChanged) {


					auto end_time = std::chrono::high_resolution_clock::now();
					auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - iCurrentWindowStartTime).count();

					//auto now = std::chrono::system_clock::to_time_t(end_time);
					
					
					//auto days = std::chrono::duration_cast<std::chrono::days>(end_time - iCurrentWindowStartTime).count();

					if (time > 0) {
						wstringstream line;
						line << time_stamp() << ",";

						if (wCurrentProcessName.length() > 0) {

							fs::path p(wCurrentProcessName);
							line << L"\"" << p.stem().wstring() << L"\",";

							if (processNameLen > 0) {
								wCurrentProcessName.assign(processName, processNameLen);
							}
							else {
								wCurrentProcessName.clear();
							}
						};

						if (wCurrentWindowName.length() > 0) {
							line << L"\"" << wCurrentWindowName << L"\",";
						}

						line << time;
						line << endl;

						auto day = localtime_xp(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())).tm_mday;
						if (day != currentDay) {
							currentDay = day;
							oFile.close();
							oFile.open(fileName, std::ios::app);
						}

						if (oFile.bad() || !oFile.good() || oFile.fail()) {
							oFile.close();
							oFile.open(fileName, std::ios::app);
						}
						if (!oFile.is_open()) {
							oFile.open(fileName, std::ios::app);
						}

						oFile << line.rdbuf();

						if (std::chrono::duration_cast<std::chrono::seconds>(end_time - lastFlush).count() > 5) {
							oFile.flush();
							lastFlush = end_time;
						}

						iCurrentWindowStartTime = end_time;
						wCurrentWindowName.assign(nextWindowName);
						nextWindowName.clear();
					}
				}
			}
			else {
				if (wCurrentWindowName.length() == 0 && nextWindowName.length() > 0) {
					wCurrentWindowName.assign(nextWindowName);
				}
				if (wCurrentProcessName.length() == 0 && processNameLen > 0) {
					wCurrentProcessName.assign(processName, processNameLen);
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

	/*HANDLE g_StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	while (WaitForSingleObject(g_StopEvent, 250) != WAIT_OBJECT_0)
	{
		wchar_t* pWindowName = NULL;
		int iWindowNameLen = 0;
		DWORD processNameLen = 0;

		HWND hActiveWindow = GetForegroundWindow();
		if (hActiveWindow != NULL) {
			int len = GetWindowTextLength(hActiveWindow);
			pWindowName = new wchar_t[len + 1];
			iWindowNameLen = GetWindowText(hActiveWindow, pWindowName,
				len + 1);
			if (iWindowNameLen == 0) {
				int error = GetLastError();
				delete[] pWindowName;
				pWindowName = NULL;
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

		if (iWindowNameLen > 0 && pWindowName != NULL) {
			int res = wcscmp(pWindowName, wCurrentWindowName.c_str());
			if (res != 0) {
				auto end_time = std::chrono::high_resolution_clock::now();
				auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - iCurrentWindowStartTime).count();

				//auto days = std::chrono::duration_cast<std::chrono::days>(end_time - iCurrentWindowStartTime).count();

				if (wCurrentWindowName.length() > 0) {
					if (wCurrentProcessName.length() > 0) {
						fs::path p(wCurrentProcessName);
						oFile << L"\"" << p.stem().wstring() << L"\",";
					};
					if (processNameLen > 0) {
						wCurrentProcessName.assign(processName, processNameLen);
					}
					else {
						wCurrentProcessName.clear();
					}
					wstring str = L"\"" + wCurrentWindowName + L"\"," + to_wstring(time) + L"\n";
					oFile.write(str.c_str(), str.length());
					oFile.flush();
				}
				iCurrentWindowStartTime = end_time;
				wCurrentWindowName.assign(pWindowName, iWindowNameLen);
				if (pWindowName) {
					delete[] pWindowName;
					pWindowName = NULL;
				}
			}
		}
		else {
			if (pWindowName) {
				delete[] pWindowName;
				pWindowName = NULL;
			}
		};
	}*/

	oFile.close();
	//CloseHandle(g_StopEvent);
}