#pragma once
#include <time.h>
#include <ctime>
#include <stdio.h>
#include <string>
#include <chrono>
#include <algorithm>
#include <vector>
#include <filesystem>



using namespace std;

wstring CurrentDate();
std::wstring time_stamp(const std::wstring&  fmt = L"%F %T");
string to_utf8(const wstring &s);
inline std::tm localtime_xp(std::time_t timer);
std::vector<std::wstring> split(const std::wstring &s, wchar_t delim);
filesystem::path GetAppDataFolderPath();
bool CheckUniqueProcess();
void replaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to);
HWND GetRealParent(HWND hWnd);