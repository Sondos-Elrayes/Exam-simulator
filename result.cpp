#include "result.h"
#include <iostream>
#include <iomanip>
#include <sqlite3.h>
#include <string>

using namespace std;

void viewExamResults(const string& username) {
    sqlite3* db;
    if (sqlite3_open("exam.db", &db) != SQLITE_OK) {
        cerr << "Failed to open database: " << sqlite3_errmsg(db) << endl;
        return;
    }

    string query = R"(
        SELECT er.reservation_id, e.exam_name, er.exam_date, er.score, er.pass_score, er.status, er.exam_code, er.rem_time
        FROM Exam_reservation er
        JOIN Exams e ON er.exam_code = e.exam_code
        WHERE er.username = ? AND er.status = 4
        ORDER BY er.exam_date DESC
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        cerr << "Failed to prepare results query\n";
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    cout << "\nYour Past Exam Results:\n";
    cout << left << setw(15) << "Reservation ID"
         << setw(30) << "Exam Name"
         << setw(15) << "Exam Date"
         << setw(10) << "Score"
         << setw(10) << "Status"
         << setw(15) << "Time Taken" << endl;
    cout << string(95, '=') << endl;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int resID = sqlite3_column_int(stmt, 0);
        const char* examName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* examDate = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        int score = sqlite3_column_int(stmt, 3);
        int passScore = sqlite3_column_int(stmt, 4);
        const char* examCode = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        int remTime = sqlite3_column_int(stmt, 7);
        string status = score >= passScore ? "PASS" : "FAIL";
        int minutes = 30 - remTime;

        stringstream resultLine;
        resultLine << left << setw(15) << resID
                   << setw(30) << examName
                   << setw(15) << examDate
                   << setw(10) << score
                   << setw(10) << status
                   << setw(15) << (to_string(minutes) + " min");
        string resultText = resultLine.str();

        cout << resultText << endl;

        // Section-wise breakdown
        string breakdownQuery = R"(
            SELECT s.section_name, erss.score
            FROM exam_running_section_score erss
            JOIN Exam_details ed ON ed.section = erss.section_code AND ed.exam_code = ?
            JOIN Sections s ON s.section_code = erss.section_code AND s.sub_code = ed.subject_code
            WHERE erss.res_id = ?
        )";

        sqlite3_stmt* sectionStmt;
        if (sqlite3_prepare_v2(db, breakdownQuery.c_str(), -1, &sectionStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(sectionStmt, 1, examCode, -1, SQLITE_STATIC);
            sqlite3_bind_int(sectionStmt, 2, resID);

            bool hasSections = false;
            while (sqlite3_step(sectionStmt) == SQLITE_ROW) {
                if (!hasSections) {
                    cout << string(15 , ' ') << "Sections:" << endl;
                    hasSections = true;
                }
                const char* sectionName = reinterpret_cast<const char*>(sqlite3_column_text(sectionStmt, 0));
                int sectionScore = sqlite3_column_int(sectionStmt, 1);
                cout << string(15 , ' ') << "- " << sectionName << ": " << sectionScore << " marks\n";
            }
            sqlite3_finalize(sectionStmt);
        }

        cout << string(95, '-') << endl; // visual separator between exams
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}
