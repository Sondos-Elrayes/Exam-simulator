#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>

bool connectToDatabase(sqlite3*& db);
bool loginUser(sqlite3* db, const std::string& username, const std::string& password);

#endif
