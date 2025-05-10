#include <iostream>
#include <string>
#include <conio.h>
#include <iomanip> // For centering output
#include <windows.h>
#include "user.h"
#include <sqlite3.h>
#include "database.h"
#include "exam.h"
#include "result.h"

using namespace std;

void setConsoleColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void centerText(const string& text, int width = 120, bool bold = false) {
    int padding = (width - text.size()) / 2;
    if (bold) setConsoleColor(15 | FOREGROUND_INTENSITY); // Bright white
    cout << string(padding > 0 ? padding : 0, ' ') << text << endl;
    if (bold) setConsoleColor(7); // Reset to default
}

void showMainMenu(const string& username) {
    int choice;

    do {
        system("cls");
        cout << endl;
        centerText("Welcome, " + username + "!", 120, true);
        centerText("==========================", 120);
        centerText("1. View My Exams", 120);
        centerText("2. View Exam Results", 120);
        centerText("3. Exit", 120);
        centerText("Enter your choice: ", 120);
        cout << string(60, ' '); // Move cursor to center-ish
        cin >> choice;

        switch (choice) {
            case 1:
                selectAndStartExam(username);
                break;
            case 2:
                system("cls");
                viewExamResults(username);
                system("pause");
                break;
            case 3:
                centerText("Goodbye!\n", 120, true);
                break;
            default:
                centerText("Invalid choice. Please try again.\n", 120, true);
        }

    } while (choice != 3);
}

int main() {
    sqlite3* db;
    if (!connectToDatabase(db)) {
        cerr << "Unable to open database. Exiting...\n";
        return 1;
    }

    system("cls");
    cout << endl;
    centerText("==================================", 120, true);
    centerText("Welcome to the Exam Simulator!", 120, true);
    centerText("==================================", 120, true);
    centerText("Please login to continue...\n", 120);

    string username, password;
    bool loggedIn = false;

    while (!loggedIn) {
        cout << endl;
        centerText("=== Login ===", 120, true);
        cout << string(50, ' ') << "Username: ";
        cin >> username;
        cout << string(50, ' ') << "Password: ";
        password = "";
        char ch;
        while ((ch = _getch()) != '\r') {
            if (ch == '\b') {
                if (!password.empty()) {
                    cout << "\b \b";
                    password.pop_back();
                }
            } else {
                password += ch;
                cout << '*';
            }
        }
        cout << endl;

        if (loginUser(db, username, password)) {
            centerText("\nLogin successful!\n", 120, true);
            Sleep(1000);
            showMainMenu(username);
        } else {
            centerText("Invalid credentials. Try again.\n", 120, true);
        }
    }

    sqlite3_close(db);
    return 0;
}
