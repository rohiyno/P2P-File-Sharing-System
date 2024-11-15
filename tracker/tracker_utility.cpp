#include "headers.h"

pair<string, string> getIpAndPortFromSocketInfo(string socketInfo)
{
    int delimeter = socketInfo.find(':');
    string clientIp = socketInfo.substr(0, delimeter);
    string clientPort = socketInfo.substr(delimeter + 1);
    return make_pair(clientIp, clientPort);
}

vector<pair<string, string>> readAddress()
{
    vector<pair<string, string>> address;
    ifstream file("tracker_info.txt");

    if (file.is_open())
    {
        string line;
        while (getline(file, line))
        {
            pair<string, string> addressPair = getIpAndPortFromSocketInfo(line);
            address.push_back(addressPair); // Push each line into the vector
        }
        file.close(); // Close the file
    }
    else
    {
        cerr << "Unable to open the file." << endl;
    }

    return address;
}

vector<pair<string, string>> readAddress(char *fileName)
{
    vector<pair<string, string>> address;

    FILE *file = fopen(fileName, "r"); // Open the file in read mode

    if (file)
    {
        char line[256]; // Assuming each line in the file is at most 255 characters
        while (fgets(line, sizeof(line), file) != nullptr)
        {
            // Remove newline character from the end of the line
            line[strcspn(line, "\n")] = '\0';

            string lineString(line);
            pair<string, string> addressPair = getIpAndPortFromSocketInfo(lineString);
            address.push_back(addressPair); // Push each line into the vector
        }

        fclose(file); // Close the file
    }
    else
    {
        cerr << "Unable to open the file." << endl;
    }

    return address;
}

int createConnection(pair<string, string> socketInfo)
{
    struct sockaddr_in serverAdress;
    createLogEntry("Client socket created");
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        createLogEntry("Socket creation failed!");
        cerr << "Socket creation failed";
        return -1;
    }
    const char *hostIp = socketInfo.first.c_str();
    serverAdress.sin_family = AF_INET;
    serverAdress.sin_port = htons(stoi(socketInfo.second));
    serverAdress.sin_addr.s_addr = inet_addr(hostIp);

    int status = connect(clientSocket, (struct sockaddr *)&serverAdress, sizeof(serverAdress));
    if (status < 0)
    {
        createLogEntry("Connection failed!");
        cerr << "Couldn't connect with the server";
        return -1;
    }
    createLogEntry("Connection Established!");
    if (socketInfo.second == "4000")
    {
        cout << "Connected to server ...🚀" << endl;
    }
    else
    {
        cout << "Connected to peer server ...🚀" << endl;
    }

    createLogEntry("Get socket:" + to_string(clientSocket));
    return clientSocket;
}

vector<string> tokenizeString(const string &inputString)
{
    vector<string> tokens;
    istringstream tokenStream(inputString);
    string token;
    while (getline(tokenStream, token, '\x1C'))
    {
        tokens.push_back(token);
    }
    return tokens;
}
