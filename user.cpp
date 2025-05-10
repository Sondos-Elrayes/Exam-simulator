#include "user.h"
#include <iostream>
#include <sqlite3.h>

using namespace std;

bool login(const string& username, const string& password) {
    sqlite3* db;
    sqlite3_stmt* stmt;
    int rc;

    rc = sqlite3_open("exam.db", &db);
    if (rc) {
        cerr << "Can't open database: " << sqlite3_errmsg(db) << endl;
        return false;
    }

    string query = "SELECT COUNT(*) FROM exam_users WHERE username = ? AND password = ?";
    rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Failed to prepare statement\n";
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    bool success = false;
    if (rc == SQLITE_ROW && sqlite3_column_int(stmt, 0) > 0) {
        success = true;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return success;
}
