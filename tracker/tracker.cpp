#include "headers.h"
bool running = true;
unordered_map<string, string> shaToFileNameMapping;
unordered_map<string, UserDetails> users;
unordered_map<int, Group> groupDetails;

int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        cout << "Invalid Arguments\n<exe> <tracker_port>" << endl;
    }
    createLogEntry("Tracker Started started");
    // int trackerNumber = stoi(argv[2]);
    vector<pair<string, string>> trackerDetails = readAddress(argv[1]);

    pair<string, string> ipAndPort = trackerDetails[0];
    int port = stoi(ipAndPort.second);
    cout << port << endl;
    thread p(startTracker, port);
    p.join();
    return 0;
}