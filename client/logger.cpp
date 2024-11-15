#include "headers.h"
sem_t logFileLock;

void createLogEntry(string logMessage)
{
    ofstream logFile("log.txt", ios_base::out | ios_base::app);
    logFile << logMessage << endl;
}
