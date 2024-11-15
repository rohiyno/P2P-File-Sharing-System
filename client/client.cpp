
#include "headers.h"

bool running = true;
std::mutex fileMutex;
unordered_map<string, FileDetails> files;
queue<unordered_map<string, FileDownloadInfo>> pendingDownloads;
queue<unordered_map<string, FileDownloadInfo>> ongoingDownloads;
queue<unordered_map<string, FileDownloadInfo>> compeletedDownloads;
unordered_map<string, string> shaToFileNameMapping;
// unordered_map<long long, vector<string>> chunkInfo;

int main(int argc, char **argv)
{

    if (argc != 3)
    {
        cout << "Provide valid argument";
        exit(0);
    }
    createLogEntry("Program started");
    string socketInfo = argv[1];
    pair<string, string> ipAndPort = getIpAndPortFromSocketInfo(socketInfo);
    int port = stoi(ipAndPort.second);
    cout << port << endl;
    thread p(startServer, port);
    p.detach();

    // while (true)
    // {
    //     vector<pair<string, string>> peerDetails;
    //     peerDetails = readAddress(argv[2]);
    //     cout
    //         << "Enter peer from whom want to take file";
    //     int n;
    //     cin >> n;

    //     string fileName;

    //     int peerSocket = createConnection(peerDetails[n]);
    //     if (peerSocket < 0)
    //     {
    //         createLogEntry("Unable to connect with peer");
    //         cerr << "Unable to connect with peer !!" << endl;
    //         continue;
    //     }
    //     cout << "Enter filename you want to receive!" << endl;
    //     cin.ignore();
    //     getline(cin, fileName);
    //     cout << "after getline" << endl;
    //     char msg[100];
    //     strcpy(msg, fileName.c_str());

    //     write(peerSocket, msg, sizeof(msg));

    //     thread downloadThread(downloadFile, peerSocket, fileName);

    //     downloadThread.join();
    // }

    char *fileName = argv[2];
    vector<pair<string, string>> peerDetails;
    peerDetails = readAddress(fileName);
    if (peerDetails.size())
    {
        thread request(makeRequest, peerDetails[0], socketInfo);
        request.join();
    }
    return 0;
}
