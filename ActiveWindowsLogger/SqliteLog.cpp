#include "stdafx.h"
#include "SqliteLog.h"
#include "./lib/sqlite/sqlite3.h"

static sqlite3* g_db = nullptr;
static sqlite3_stmt* g_insert = nullptr;

bool db_isOpen() {
	return g_db != nullptr;
}

bool db_open(const std::wstring& dbPath) {
	if (g_db) {
		return true;
	}
	// On Windows wchar_t is UTF-16, so we can use the native open16 entry point.
	if (sqlite3_open16(dbPath.c_str(), &g_db) != SQLITE_OK) {
		if (g_db) {
			sqlite3_close(g_db);
			g_db = nullptr;
		}
		return false;
	}

	const char* createSql =
		"CREATE TABLE IF NOT EXISTS activity ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"timestamp TEXT NOT NULL,"
		"program TEXT,"
		"window_title TEXT,"
		"time_ms INTEGER,"
		"computer TEXT);";
	if (sqlite3_exec(g_db, createSql, nullptr, nullptr, nullptr) != SQLITE_OK) {
		sqlite3_close(g_db);
		g_db = nullptr;
		return false;
	}

	// Index to speed up common queries by day / program.
	sqlite3_exec(g_db,
		"CREATE INDEX IF NOT EXISTS idx_activity_ts ON activity(timestamp);",
		nullptr, nullptr, nullptr);

	// Better throughput, still crash-safe.
	sqlite3_exec(g_db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
	sqlite3_exec(g_db, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);

	const char* insSql =
		"INSERT INTO activity(timestamp,program,window_title,time_ms,computer) "
		"VALUES(?,?,?,?,?);";
	if (sqlite3_prepare_v2(g_db, insSql, -1, &g_insert, nullptr) != SQLITE_OK) {
		sqlite3_close(g_db);
		g_db = nullptr;
		return false;
	}
	return true;
}

void db_write(const std::string& timestamp, const std::string& program,
	const std::string& windowTitle, long long timeMs, const std::string& computer) {
	if (!g_db || !g_insert) {
		return;
	}
	sqlite3_reset(g_insert);
	sqlite3_clear_bindings(g_insert);
	sqlite3_bind_text(g_insert, 1, timestamp.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(g_insert, 2, program.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(g_insert, 3, windowTitle.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int64(g_insert, 4, (sqlite3_int64)timeMs);
	sqlite3_bind_text(g_insert, 5, computer.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_step(g_insert);
}

void db_close() {
	if (g_insert) {
		sqlite3_finalize(g_insert);
		g_insert = nullptr;
	}
	if (g_db) {
		sqlite3_close(g_db);
		g_db = nullptr;
	}
}
