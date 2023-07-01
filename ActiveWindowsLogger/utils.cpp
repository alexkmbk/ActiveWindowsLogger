#include "stdafx.h"
#include <shlobj_core.h>
#include "utils.h"

inline std::tm localtime_xp(std::time_t timer)
{
	std::tm bt{};
#if defined(__unix__)
	localtime_r(&timer, &bt);
#elif defined(_MSC_VER)
	localtime_s(&bt, &timer);
#else
	static std::mutex mtx;
	std::lock_guard<std::mutex> lock(mtx);
	bt = *std::localtime(&timer);
#endif
	return bt;
}

wstring CurrentDate() {
	auto bt = localtime_xp(std::time(0));
	wchar_t date[64];
	swprintf_s(date, L"%02d-%02d-%04d", bt.tm_mday, bt.tm_mon + 1, bt.tm_year +
		1900);

	return wstring(date);
}

// default = "YYYY-MM-DD HH:MM:SS"
// %F = 2001-08-23
// %T = 14:55:02

std::wstring time_stamp(const std::wstring& fmt)
{
	auto bt = localtime_xp(std::time(0));
	wchar_t buf[64];
	std::wcsftime(buf, sizeof(buf), fmt.c_str(), &bt);
	return wstring(buf);
}

string to_utf8(const wstring &s)
{
	string utf8;
	int len = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), s.length(), NULL, 0, NULL, NULL);
	if (len > 0)
	{
		utf8.resize(len);
		WideCharToMultiByte(CP_UTF8, 0, s.c_str(), s.length(), &utf8[0], len, NULL, NULL);
	}
	return utf8;
}

#include <string>
#include <sstream>
#include <vector>
std::vector<std::wstring> split(const std::wstring &s, wchar_t delim) {
	std::wstringstream ss(s);
	std::wstring item;
	std::vector<std::wstring> elems;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

filesystem::path GetAppDataFolderPath() {
	filesystem::path path;
	wchar_t* wcPath = NULL;
    auto res = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0,0, &wcPath);
	if (res == S_OK) {
		path.assign(wcPath);
		CoTaskMemFree(wcPath);
	}
	return path;

}

void replaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to) {
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

//
// Returns the real parent window
// Same as GetParent(), but doesn't return the owner
//
HWND GetRealParent(HWND hWnd)
{
	HWND hParent;

	hParent = GetAncestor(hWnd, GA_PARENT);
	if (!hParent || hParent == GetDesktopWindow())
		return NULL;

	return hParent;
}

bool IsWindowsVersionOrGreater(WORD wMajorVersion, WORD wMinorVersion)
{
	OSVERSIONINFOEXW osvi = {};
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	DWORDLONG        const dwlConditionMask = VerSetConditionMask(
		VerSetConditionMask(
			VerSetConditionMask(
				0, VER_MAJORVERSION, VER_GREATER_EQUAL),
			VER_MINORVERSION, VER_GREATER_EQUAL),
		VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);

	osvi.dwMajorVersion = wMajorVersion;
	osvi.dwMinorVersion = wMinorVersion;

	return VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask) != FALSE;
}