#include "stdafx.h"
#include "utils.h"

/*wstring CurrentDate() {
	time_t t = time(0);
	tm* lt = localtime(&t);
	wchar_t date[11];
	swprintf_s(date, L"%02d.%02d.%04d", lt->tm_mday, lt->tm_mon + 1, lt->tm_year +
		1900);

	return wstring(date);
}*/

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

// default = "YYYY-MM-DD HH:MM:SS"
std::wstring time_stamp()
{
	auto bt = localtime_xp(std::time(0));
	//char buf[64];
	//return { buf, std::strftime(buf, sizeof(buf), fmt.c_str(), &bt) };

	wchar_t date[64];

	swprintf_s(date, L"%02d.%02d.%04d", bt.tm_mday, bt.tm_mon + 1, bt.tm_year +
		1900);

	return wstring(date);
}
