#include "headers.h"

pair<string, string> getIpAndPortFromSocketInfo(string socketInfo)
{
    int delimeter = socketInfo.find(':');
    string clientIp = socketInfo.substr(0, delimeter);
    string clientPort = socketInfo.substr(delimeter + 1);
    return make_pair(clientIp, clientPort);
}

vector<pair<string, string>> readAddress(char *fileName)
{
    vector<pair<string, string>> address;
    ifstream file(fileName);

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
    cout << "Connected to server ...🚀" << endl;
    createLogEntry("Get socket:" + to_string(clientSocket));
    return clientSocket;
}

vector<string> tokenizeCommand(string command, char delimiter)
{
    vector<string> tokens; // Vector to store tokens

    istringstream iss(command); // Create a stringstream from the input string

    string token;
    while (getline(iss, token, delimiter))
    {
        tokens.push_back(token); // Push each token into the vector using the specified delimiter
    }
    return tokens; // Return the vector of tokens
}

long long getFileSize(const string &filename)
{
    struct stat fileStat;
    if (stat(filename.c_str(), &fileStat) == 0)
    {
        // File size is stored in the st_size member of the fileStat structure
        return fileStat.st_size;
    }
    else
    {
        // Return -1 to indicate error
        return -1;
    }
}

long long getTotalChunksOfFile(const string &filename)
{
    struct stat fileStat;
    if (stat(filename.c_str(), &fileStat) == 0)
    {
        // File size is stored in the st_size member of the fileStat structure
        long long fileSize = fileStat.st_size;
        long long totalChunks = ceil((float)fileSize / 524288);
        return totalChunks;
    }
    else
    {
        // Return -1 to indicate error
        return -1;
    }
}
