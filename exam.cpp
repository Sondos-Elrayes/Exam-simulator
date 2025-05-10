#include "exam.h"
#include <iostream>
#include <sqlite3.h>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>
#include <random>
#include <algorithm>
#include <map>
#include <set>
#include <tuple>
#include <future>
#include <atomic>
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <limits>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <map>
#include <tuple>
#include <atomic>
#include <string>

using namespace std;
using namespace chrono;

atomic<bool> examRunning(true);

void countdownTimer(int durationInSec) {
    while (examRunning && durationInSec > 0) {
        int minutes = durationInSec / 60;
        int seconds = durationInSec % 60;
        cout << "\rTime Remaining: " << minutes << ":" << (seconds < 10 ? "0" : "") << seconds << " " << flush;
        this_thread::sleep_for(chrono::seconds(1));
        durationInSec--;
    }
}

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void displayExamResultsSummary(const map<string, tuple<string, int, int>>& sectionDetails, int totalScore, int passScore) {
    clearScreen();
    cout << left << setw(20) << "Subject" << setw(25) << "Section" << setw(15) << "no_questions" << setw(10) << "correct" << " %" << endl;
    cout << string(80, '-') << endl;

    int totalQuestions = 0;
    int totalCorrect = 0;

    for (map<string, tuple<string, int, int>>::const_iterator it = sectionDetails.begin(); it != sectionDetails.end(); ++it) {
        string sectionCode = it->first;
        string subject = get<0>(it->second);
        string section = sectionCode;
        int total = get<1>(it->second);
        int correct = get<2>(it->second);

        cout << left << setw(20) << subject
             << setw(25) << section
             << setw(15) << total
             << setw(10) << correct
             << (total == 0 ? "0%" : to_string(correct ) + "%")
             << endl;

        totalQuestions += total;
        totalCorrect += correct;
        this_thread::sleep_for(chrono::milliseconds(300)); // Slow down line-by-line output
    }

    int percentage = (totalQuestions == 0) ? 0 : totalCorrect ;
    cout << "\nOverall Score: " << percentage << "%  -->  " << (totalScore >= passScore ? "Pass" : "Fail") << endl;
    if (totalScore >= passScore) {
        cout << "\nCongratulations! You passed the exam.\n";
    } else {
        cout << "\nBetter luck next time.\n";
    }
    cout << string(80, '-') << endl;

    cout << "\nPress Enter to return to menu...";
    cin.ignore();
    cin.get();
}


bool showExamSummary(int reservationID) {
    sqlite3* db;
    if (sqlite3_open("exam.db", &db) != SQLITE_OK) {
        cerr << "Failed to open database." << endl;
        return false;
    }

    string examCode, examName;
    int examTime, maxScore, passScore;

    string query = R"(
        SELECT e.exam_code, e.exam_name, e.exam_time, e.max_score, e.pass_score
        FROM Exam_reservation er
        JOIN Exams e ON er.exam_code = e.exam_code
        WHERE er.reservation_id = ?
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        cerr << "Failed to prepare exam summary query." << endl;
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_int(stmt, 1, reservationID);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        examCode = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        examName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        examTime = sqlite3_column_int(stmt, 2);
        maxScore = sqlite3_column_int(stmt, 3);
        passScore = sqlite3_column_int(stmt, 4);
    } else {
        cerr << "No exam found for reservation ID." << endl;
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return false;
    }
    sqlite3_finalize(stmt);

    clearScreen();

    cout << "\n==============================================================\n";
    cout << left << setw(20) << "Exam Code:" << examCode << endl;
    cout << left << setw(20) << "Exam Name:" << examName << endl;
    cout << left << setw(20) << "Exam Time:" << examTime << " minutes" << endl;
    cout << left << setw(20) << "Max Score:" << maxScore << endl;
    cout << left << setw(20) << "Pass Score:" << passScore << endl;
    cout << "\n" << left << setw(20) << "Subject" << setw(25) << "Section" << "No. of Questions" << endl;
    cout << string(65, '-') << endl;

    string sectionQuery = R"(
        SELECT s.sub_code, s.section_name, ed.no_questions
        FROM Exam_details ed
        JOIN Sections s ON ed.section = s.section_code AND ed.subject_code = s.sub_code
        WHERE ed.exam_code = ?
    )";

    if (sqlite3_prepare_v2(db, sectionQuery.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, examCode.c_str(), -1, SQLITE_STATIC);
        int totalQuestions = 0;

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            string subject = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            string sectionName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            int count = sqlite3_column_int(stmt, 2);
            totalQuestions += count;
            cout << left << setw(20) << subject << setw(25) << sectionName << count << endl;
        }
        cout << "\n" << left << setw(45) << "Total Number of Questions:" << totalQuestions << endl;
        sqlite3_finalize(stmt);
    }

    cout << "\nIf you're ready to start, press 1. To cancel, press q: ";
    string input;
    cin >> input;

    sqlite3_close(db);
    if (input == "1") return true;
    else {
        cout << "Exam cancelled by user.\n";
        return false;
    }
}



bool startExam(int reservationID, const string& username) {
    sqlite3* db;
    if (sqlite3_open("exam.db", &db) != SQLITE_OK) {
        cerr << "Failed to open database: " << sqlite3_errmsg(db) << endl;
        return false;
    }

    string examCode;
    int examTime;
    int passScore;
    string examName;
    {
        string query = "SELECT er.exam_code, e.exam_time, e.exam_name, e.pass_score FROM Exam_reservation er "
                      "JOIN Exams e ON er.exam_code = e.exam_code WHERE er.reservation_id = ?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            cerr << "Failed to prepare exam info query\n";
            sqlite3_close(db);
            return false;
        }
        sqlite3_bind_int(stmt, 1, reservationID);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            examCode = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            examTime = sqlite3_column_int(stmt, 1);
            examName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            passScore = sqlite3_column_int(stmt, 3);
        } else {
            cerr << "Reservation not found\n";
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return false;
        }
        sqlite3_finalize(stmt);
    }

    struct SectionInfo {
        string sectionCode;
        int no_questions;
        string sectionName;
    };

    vector<SectionInfo> sections;
    {
        string query = R"(
            SELECT ed.section, ed.no_questions, s.section_name
            FROM Exam_details ed
            JOIN Sections s ON ed.section = s.section_code AND ed.subject_code = s.sub_code
            WHERE ed.exam_code = ?
        )";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            cerr << "Failed to prepare sections query\n";
            sqlite3_close(db);
            return false;
        }
        sqlite3_bind_text(stmt, 1, examCode.c_str(), -1, SQLITE_STATIC);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            SectionInfo si;
            si.sectionCode = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            si.no_questions = sqlite3_column_int(stmt, 1);
            si.sectionName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            sections.push_back(si);
        }
        sqlite3_finalize(stmt);
    }

    if (!showExamSummary(reservationID)) {
        sqlite3_close(db);
        return false;
    }

    map<string, vector<int>> sectionQuestions;
    for (const auto& sec : sections) {
        string query = R"(
            SELECT q.question_no
            FROM Questions q
            JOIN Exam_details ed ON ed.subject_code = q.sub_code AND ed.section = q.section_code
            WHERE ed.exam_code = ? AND ed.section = ?
            ORDER BY RANDOM() LIMIT ?;
        )";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            cerr << "Failed to prepare random question query\n";
            sqlite3_close(db);
            return false;
        }

        sqlite3_bind_text(stmt, 1, examCode.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, sec.sectionCode.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, sec.no_questions);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int qno = sqlite3_column_int(stmt, 0);
            sectionQuestions[sec.sectionCode].push_back(qno);
        }

        sqlite3_finalize(stmt);
    }

    auto start = chrono::steady_clock::now();
    map<string, int> sectionScores;
    int totalScore = 0;
    vector<pair<string, int>> allQuestions;

    for (const auto& sec : sectionQuestions) {
        for (int qid : sec.second) {
            allQuestions.emplace_back(sec.first, qid);
            sectionScores[sec.first] = 0;
        }
    }

    int current = 0;
    vector<int> userAnswers(allQuestions.size(), -1);
    int totalExamTimeInSec = examTime * 60;
    future<void> timer = async(launch::async, countdownTimer, totalExamTimeInSec);

    while (true) {
        auto now = chrono::steady_clock::now();
        int elapsed = chrono::duration_cast<chrono::seconds>(now - start).count();
        int remainingSec = totalExamTimeInSec - elapsed;
        if (remainingSec <= 0) {
            cout << "\nTime is up! Submitting exam...\n";
            break;
        }

        string sectionCode = allQuestions[current].first;
        int qid = allQuestions[current].second;

        string qText;
        vector<pair<int, string>> answers;
        int correctAnswer = -1;

        string qQuery = "SELECT question_header FROM Questions WHERE question_no = ?";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, qQuery.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, qid);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            qText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        }
        sqlite3_finalize(stmt);

        string aQuery = "SELECT ans_id, ans_text, iscorrect FROM Question_ans WHERE question_no = ?";
        sqlite3_prepare_v2(db, aQuery.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, qid);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int aid = sqlite3_column_int(stmt, 0);
            string text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            int isCorrect = sqlite3_column_int(stmt, 2);
            answers.emplace_back(aid, text);
            if (isCorrect == 1) correctAnswer = aid;
        }
        sqlite3_finalize(stmt);

        system("cls");
        int min = remainingSec / 60, sec = remainingSec % 60;
        cout << "Time Remaining: " << min << ":" << (sec < 10 ? "0" : "") << sec << "\n";
        cout << "\n--- Section: " << sectionCode << " ---\n";
        cout << "Q" << current + 1 << ": " << qText << endl;
        for (size_t i = 0; i < answers.size(); ++i) {
            cout << i + 1 << ". " << answers[i].second << endl;
        }
        if (userAnswers[current] != -1) {
            cout << "[Previously selected answer: " << userAnswers[current] + 1 << "]\n";
        }

        cout << "\nChoose an option (1-4), n (next), p (previous)";
        if (current == allQuestions.size() - 1) cout << ", s (submit)";
        cout << ":\n> ";

        string action;
        getline(cin >> ws, action);

        if (action == "n" && current < allQuestions.size() - 1) {
            current++;
        } else if (action == "p" && current > 0) {
            current--;
        } else if (action == "s" && current == allQuestions.size() - 1) {
            break;
        } else {
            try {
                int ans = stoi(action);
                if (ans >= 1 && ans <= 4) {
                    userAnswers[current] = ans - 1;
                }
            } catch (...) {
                cout << "Invalid input.\n";
                this_thread::sleep_for(chrono::seconds(1));
            }
        }
    }

    examRunning = false;
    timer.wait();

    for (size_t i = 0; i < allQuestions.size(); ++i) {
        if (userAnswers[i] == -1) continue;

        string sectionCode = allQuestions[i].first;
        int qid = allQuestions[i].second;

        string aQuery = "SELECT ans_id, iscorrect FROM Question_ans WHERE question_no = ?";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, aQuery.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, qid);

        vector<int> ansIDs;
        vector<int> correctFlags;

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ansIDs.push_back(sqlite3_column_int(stmt, 0));
            correctFlags.push_back(sqlite3_column_int(stmt, 1));
        }
        sqlite3_finalize(stmt);

        if (userAnswers[i] < (int)ansIDs.size() && correctFlags[userAnswers[i]] == 1) {
            totalScore += 10;
            sectionScores[sectionCode] += 10;
        }
    }

    auto end = chrono::steady_clock::now();
    int timeTaken = chrono::duration_cast<chrono::seconds>(end - start).count();

    system("cls");
    cout << "================== Exam Result ==================\n";
    cout << "Exam Name   : " << examName << endl;
    cout << "Username    : " << username << endl;
    cout << "Total Score : " << totalScore << "/100\n";
    cout << "Status      : " << (totalScore >= passScore ? "PASS" : "FAIL") << endl;
    cout << "Time Taken  : " << (timeTaken / 60) << " min " << (timeTaken % 60) << " sec\n";
    cout << "------------------------------------------------\n";
    cout << "Section-wise Breakdown:\n";
    for (const auto& entry : sectionScores) {
        cout << "  - Section " << entry.first << ": " << entry.second << " marks\n";
    }
    if (totalScore >= passScore) {
        cout << "\nCongratulations! You passed the exam!\n";
    } else {
        cout << "\nBetter luck next time.\n";
    }
    cout << "================================================\n";

    string updateQuery = R"(
        UPDATE Exam_reservation
        SET score = ?, status = 4, rem_time = ?
        WHERE reservation_id = ?;
    )";
    sqlite3_stmt* updateStmt;
    if (sqlite3_prepare_v2(db, updateQuery.c_str(), -1, &updateStmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(updateStmt, 1, totalScore);
        sqlite3_bind_int(updateStmt, 2, (examTime * 60 - timeTaken) / 60);
        sqlite3_bind_int(updateStmt, 3, reservationID);

        if (sqlite3_step(updateStmt) != SQLITE_DONE) {
            cerr << "Failed to update exam result.\n";
        }
    }
    sqlite3_finalize(updateStmt);

    for (const auto& entry : sectionScores) {
        string insertQuery = "INSERT INTO exam_running_section_score (res_id, section_code, score) VALUES (?, ?, ?)";
        sqlite3_stmt* insertStmt;
        if (sqlite3_prepare_v2(db, insertQuery.c_str(), -1, &insertStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(insertStmt, 1, reservationID);
            sqlite3_bind_text(insertStmt, 2, entry.first.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(insertStmt, 3, entry.second);
            if (sqlite3_step(insertStmt) != SQLITE_DONE) {
                cerr << "Failed to insert section score: " << entry.first << endl;
            }
        }
        sqlite3_finalize(insertStmt);
    }
    map<string, tuple<string, int, int>> sectionDetails;

        for (const auto& sec : sections) {
            string secCode = sec.sectionCode;
            string secName = sec.sectionName;
            int total = sec.no_questions;
            int correct = sectionScores[secCode];
            sectionDetails[secCode] = make_tuple(secName, total, correct);
        }


        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        displayExamResultsSummary(sectionDetails, totalScore, passScore);

    sqlite3_close(db);
    return true;
}





bool selectAndStartExam(const string& username) {
    sqlite3* db;
    sqlite3_stmt* stmt;
    int rc = sqlite3_open("exam.db", &db);
    if (rc != SQLITE_OK) {
        cerr << "Failed to connect to the database: " << sqlite3_errmsg(db) << endl;
        return false;
    }

    string query = R"(
        SELECT er.reservation_id, e.exam_name, er.exam_date, s.status_description, er.status
        FROM Exam_reservation er
        JOIN Exams e ON er.exam_code = e.exam_code
        JOIN reserv_exam_status s ON er.status = s.exam_status
        WHERE er.username = ?
        ORDER BY er.exam_date DESC;
    )";

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        cerr << "Failed to prepare statement\n";
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    cout << "\nYour Exams:\n";
    cout << left << setw(15) << "Reservation ID"
         << setw(30) << "Exam Name"
         << setw(15) << "Exam Date"
         << setw(15) << "Status" << endl;
    cout << string(75, '-') << endl;

    vector<int> startableIds;
    bool hasResults = false;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        hasResults = true;
        int resID = sqlite3_column_int(stmt, 0);
        const char* examName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* examDate = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        int statusCode = sqlite3_column_int(stmt, 4);

        cout << left << setw(15) << resID
             << setw(30) << examName
             << setw(15) << examDate
             << setw(15) << status << endl;

        if (statusCode == 1 || statusCode == 2) {
            startableIds.push_back(resID);
        }
    }

    sqlite3_finalize(stmt);

    if (!hasResults) {
        cout << "\nNo exams found.\n";
        sqlite3_close(db);
        return false;
    }


    int selectedId;
    while (true) {
        cout << "\nEnter Reservation ID to start the exam (or 0 to cancel): ";
        cin >> selectedId;

        if (selectedId == 0) {
            cout << "Exam cancelled.\nPress Enter to return to menu...";
            cin.ignore();
            cin.get();
            sqlite3_close(db);
            return false;
        }

        if (find(startableIds.begin(), startableIds.end(), selectedId) != startableIds.end()) {
            sqlite3_close(db);
            return startExam(selectedId, username);
        }

        cout << "You cannot start this exam. It is either finished or currently not reserved.\n";
        cout << "Try again...\n";
    }
}
