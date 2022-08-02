#include "stdafx.h"
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
	swprintf_s(date, L"%02d.%02d.%04d", bt.tm_mday, bt.tm_mon + 1, bt.tm_year +
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
