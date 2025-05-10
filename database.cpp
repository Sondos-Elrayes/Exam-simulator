#include "database.h"
#include <iostream>
using namespace std;

bool connectToDatabase(sqlite3*& db){
    int rc = sqlite3_open("exam.db", &db);
    if (rc) {
        cerr << "Failed to connect to database: " << sqlite3_errmsg(db) << endl;
        return false;
    }
    return true;
}

bool loginUser(sqlite3* db, const std::string& username, const std::string& password) {
    const char* sql = "SELECT * FROM exam_users WHERE username = ? AND password = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_ROW;
}
