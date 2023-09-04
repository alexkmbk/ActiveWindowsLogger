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

#include <fcntl.h>
//#include <semaphore.h>

#include <unistd.h>
#include <sys/file.h>

using std::string;
using std::wstring;

std::wstring CurrentDate();
std::string time_stamp(const std::string&  fmt = "%F %T");
string to_utf8(const wstring &s);
inline std::tm localtime_xp(std::time_t timer);
std::vector<std::string> split(const std::string &s, char delim);
std::filesystem::path GetAppDataFolderPath();
void replaceAll(std::string& str, const std::string& from, const std::string& to);
//HWND GetRealParent(HWND hWnd);
//bool IsWindowsVersionOrGreater(WORD wMajorVersion, WORD wMinorVersion);
std::string GetLastErrorAsString();

int processExist(const string processFullPath);
void releaseLockFile(int lockFileID, const string processFullPath);