#pragma once
#include <string>

// Optional SQLite logging backend.
// All strings (except dbPath) are expected to be UTF-8.

bool db_open(const std::wstring& dbPath);
bool db_isOpen();
void db_write(const std::string& timestamp, const std::string& program,
	const std::string& windowTitle, long long timeMs, const std::string& computer);
void db_close();
