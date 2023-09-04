#pragma once
#include <time.h>
#include <ctime>
#include <stdio.h>
#include <string>
#include <chrono>
#include <algorithm>
#include <vector>
#include <filesystem>
#include <cwchar>



using namespace std;

wstring CurrentDate();
std::string time_stamp(const std::string&  fmt = "%F %T");
string to_utf8(const wstring &s);
inline std::tm localtime_xp(std::time_t timer);
std::vector<std::string> split(const std::string &s, char delim);
filesystem::path GetAppDataFolderPath();
void replaceAll(std::string& str, const std::string& from, const std::string& to);
//HWND GetRealParent(HWND hWnd);
//bool IsWindowsVersionOrGreater(WORD wMajorVersion, WORD wMinorVersion);
std::string GetLastErrorAsString();