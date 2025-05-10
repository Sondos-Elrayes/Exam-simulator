#ifndef EXAM_H
#define EXAM_H

#include <string>
using namespace std;

bool selectAndStartExam(const string& username);
bool startExam(int reservationId, const string& username);
bool showExamSummary(int reservationID);



#endif

