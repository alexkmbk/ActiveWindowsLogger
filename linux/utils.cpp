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

wstring CurrentDate()
{
	auto bt = localtime_xp(std::time(0));
	wchar_t date[64];
#ifdef _WIN32
	swprintf_s(date, L"%02d-%02d-%04d", bt.tm_mday, bt.tm_mon + 1, bt.tm_year + 1900);
#else
	// swprintf(date, L"%02d-%02d-%04d", bt.tm_mday, bt.tm_mon + 1, bt.tm_year + 1900);
	wcsftime(date, sizeof(date), L"%d-%m-%Y", &bt);
#endif

	return wstring(date);
}

// default = "YYYY-MM-DD HH:MM:SS"
// %F = 2001-08-23
// %T = 14:55:02

std::string time_stamp(const std::string& fmt)
{
	auto bt = localtime_xp(std::time(0));
	char buf[64];
	std::strftime(buf, sizeof(buf), fmt.c_str(), &bt);
	return string(buf);
}

string to_utf8(const wstring &s)
{
	string utf8;
#ifdef _WIN32
	int len = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), s.length(), NULL, 0, NULL, NULL);
	if (len > 0)
	{
		utf8.resize(len);
		WideCharToMultiByte(CP_UTF8, 0, s.c_str(), s.length(), &utf8[0], len, NULL, NULL);
	}
#endif
	return utf8;
}

#include <string>
#include <sstream>
#include <vector>
std::vector<std::string> split(const std::string &s, char delim) {
	std::stringstream ss(s);
	std::string item;
	std::vector<std::string> elems;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

#ifdef _WIN32
#include <shlobj_core.h>
#endif
std::filesystem::path GetAppDataFolderPath()
{
	std::filesystem::path path;
	wchar_t *wcPath = NULL;

#ifdef _WIN32
	auto res = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, 0, &wcPath);
	if (res == S_OK)
	{
		path.assign(wcPath);
		CoTaskMemFree(wcPath);
	}
#else
	const char *homeDir = getenv("HOME");
	path.assign(homeDir);
#endif

return path;
}

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

int processExist(const string processFullPath)
{
	// Путь к файлу блокировки (можно выбрать любой путь)
	const string lockFile = "/tmp" / std::filesystem::path(processFullPath + ".lock").stem();

	// Попытка открыть файл блокировки
	int lockFileDescriptor = open(lockFile.c_str(), O_RDWR | O_CREAT, 0644);

	// Попытка установить эксклюзивную блокировку на файле
	if (lockf(lockFileDescriptor, F_TLOCK, 0) == -1)
	{
		// Файл блокировки уже заблокирован, что может указывать на запущенную программу
		close(lockFileDescriptor);
		return 0;
	}
	return lockFileDescriptor;
}

void releaseLockFile(int lockFileID, const string processFullPath)
{
	if (lockFileID != 0) {
	lockf(lockFileID, F_ULOCK, 0);
	close(lockFileID);
	}

	const string lockFile = "/tmp" / std::filesystem::path(processFullPath + ".lock").stem();

	// Удаление файла блокировки
	unlink(lockFile.c_str());
}