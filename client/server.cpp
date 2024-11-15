#include "headers.h"

// Mutex for file operations
// char *getFilechunk(string filePath, long long chunkNo)
// {
//     std::lock_guard<std::mutex> lock(fileMutex);
//     char *fp = new char[filePath.length() + 1];
//     strcpy(fp, filePath.c_str());
//     cout << "sha: " << getFileHash(fp) << endl;
//     // Open the file using open system call
//     const int bufferSize = 524288; // 512 KB chunks
//     char *buffer = new char[bufferSize];
//     int fileDescriptor = open(filePath.c_str(), O_RDONLY, S_IRWXU);
//     if (fileDescriptor != -1)
//     {
//         // Get the size of the file
//         struct stat fileStat;
//         fstat(fileDescriptor, &fileStat);
//         size_t fileSize = fileStat.st_size;
//         int noOfBytes = 524288;
//         // Define buffer size for reading and sending data
//         if (524288 * chunkNo + 524288 > fileSize)
//         {
//             noOfBytes = fileSize - 524288 * chunkNo;
//         }
//         lseek(fileDescriptor, 0, SEEK_SET);
//         lseek(fileDescriptor, 524288 * chunkNo, SEEK_SET);
//         int readByte = read(fileDescriptor, buffer, noOfBytes);
//         cout << "chunkNo: " << chunkNo << "read:" << readByte << endl;

//         return buffer;
//     }
//     delete[] buffer;
//     return nullptr;
// }

pair<char *, int> getFilechunk(string filePath, long long chunkNo)
{
    std::lock_guard<std::mutex> lock(fileMutex);

    // Open the file using open system call
    int fileDescriptor = open(filePath.c_str(), O_RDONLY);
    if (fileDescriptor != -1)
    {
        // Get the size of the file
        struct stat fileStat;
        fstat(fileDescriptor, &fileStat);
        size_t fileSize = fileStat.st_size;

        // Calculate the start position and size for the current chunk
        off_t startPosition = 524288 * chunkNo;
        off_t endPosition = startPosition + 524288;
        if (endPosition > fileSize)
        {
            endPosition = fileSize; // Adjust end position if it exceeds the file size
        }

        // Calculate the size of the buffer
        size_t bufferSize = endPosition - startPosition;
        char *buffer = new char[bufferSize];

        // Read the chunk from the file
        ssize_t bytesRead = pread(fileDescriptor, buffer, bufferSize, startPosition);

        // Close the file descriptor
        cout << "peer side byte read:" << bytesRead << " " << chunkNo << "buuf size: " << bufferSize << endl;
        // for (size_t i = 0; i < bytesRead; i++)
        // {
        //     if (buffer[i] == '\0')
        //     {
        //         cout << "contains NULL$$$" << endl;
        //         break;
        //     }
        // }
        // cout << "Not contains" << endl;

        close(fileDescriptor);

        if (bytesRead == -1)
        {
            delete[] buffer;
            return {nullptr, 0}; // Error occurred while reading the file
        }

        return {buffer, bytesRead};
    }

    return {nullptr, 0}; // Error occurred while opening the file
}

void handleClientRequest(int port, int serverPort)
{

    const int bufferSize = 32768; // 512 KB chunks
    const int fileBufferSize = 524288;
    char *buffer = new char[bufferSize];
    // char *fileBuffer = new char[fileBufferSize];
    buffer[0] = '\0';
    if (read(port, buffer, bufferSize) == 0)
    {
        createLogEntry("Peer Terminated Abruptly!!!😞");
        cout << "Peer Terminated Abruptly!!!😞" << endl;
    }

    cout << "Peer connected 😊:" << buffer << endl;

    string peerRequest(buffer);
    vector<string> tokens = tokenizeCommand(peerRequest, ' ');

    if (tokens[0] == "GET_AVAILABLE_CHUNK_INFO")
    {
        string fileName = shaToFileNameMapping[tokens[1]];

        string message = "";
        if (files.find(fileName) != files.end())
        {
            FileDetails file = files[fileName];
            stringstream ss;
            ss << file.fileName << " ";
            ss << file.filePath << " ";
            ss << file.fileSize << " ";
            // cout << "inside if";
            for (const auto &pair : file.chunksHash)
            {
                ss << pair.first << "&";
            }

            ss.str();
            message = ss.str();
            message.pop_back();
            message = "127.0.0.1:" + to_string(serverPort) + " " + message;

            strcpy(buffer, message.c_str());
            send(port, buffer, bufferSize, 0);
        }
        else
        {
            message = "error Cannot find file😔!!";
            strcpy(buffer, message.c_str());
            send(port, buffer, bufferSize, 0);
        }
    }

    if (tokens[0] == "GIVE_CHUNK")
    {
        string fileSha = tokens[1];
        long long chunkNo = stoll(tokens[3]);
        string receivedFileName = tokens[2];
        string message = "";
        string fileName = shaToFileNameMapping[fileSha];
        if (files.find(fileName) == files.end())
        {
            message = "error Cannot find chunk!!";
            strcpy(buffer, message.c_str());
            send(port, buffer, bufferSize, 0);
            delete[] buffer;
            return;
        }

        FileDetails file = files[fileName];
        long long noOfChunks = file.numberOfChunks;
        string filePath = file.filePath;
        if (chunkNo > noOfChunks)
        {
            message = "error Cannot find chunk!!";
            strcpy(buffer, message.c_str());
            send(port, buffer, bufferSize, 0);
            delete[] buffer;
            return;
        }
        string chunkHash = file.chunksHash[chunkNo];
        pair<char *, int> fileChunk = getFilechunk(filePath, chunkNo);
        char *fileBuffer = fileChunk.first;
        // if (fileBuffer == nullptr)
        // {
        //     cout << "fileBuf is nullptr" << endl;
        // }
        // for (size_t i = 0; i < fileChunk.second; i++)
        // {
        //     if (fileBuffer[i] == '\0')
        //     {
        //         cout << "contains NULL$$$" << endl;
        //         break;
        //     }
        // }
        // cout << "Not contains" << endl;
        // cout << "chunk.second" << fileChunk.second << endl;

        string chunkSha = chunkHash;
        chunkSha = "hash:" + chunkHash;
        // cout << "chunkSHa: " << chunkSha;
        strcpy(buffer, chunkSha.c_str());

        send(port, buffer, 1024, 0);
        read(port, buffer, bufferSize);
        cout << buffer << endl;
        // int bytesSend = send(port, fileBuffer, fileChunk.second, 0);
        // cout << "byteSend: " << bytesSend;
        int totalBytesSent = 0;
        int chunkSize = 32768;
        while (totalBytesSent < fileChunk.second)
        {
            int remainingBytes = fileChunk.second - totalBytesSent;
            int bytesToSend = min(remainingBytes, chunkSize);
            int bytesSent = send(port, fileBuffer + totalBytesSent, bytesToSend, 0);
            if (bytesSent <= 0)
            {
                // Handle error if send fails
                // ...
                break;
            }
            totalBytesSent += bytesSent;
        }

        cout << "Total bytes sent: " << totalBytesSent << endl;
        delete[] fileBuffer;
        delete[] buffer;
    }
}

void startServer(int port)
{
    createLogEntry("staring server thread");

    int serverSocket, clientSocket;
    sockaddr_in serverAddress, clientAddress;
    socklen_t clientAddrLen = sizeof(clientAddress);

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        cerr << "Error: Could not create socket." << endl;
        return;
    }

    int option = 1;
    if ((setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option))) == -1)
    {
        cerr << "Error: Could not set SO_REUSEADDR option." << endl;
        close(serverSocket);
        return;
    }
    // Set up server address structure
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if ((::bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress))) == -1)
    {
        cerr << "Error: Could not bind to port " << port << "." << endl;
        close(serverSocket);
        return;
    }

    // Listen for incoming connections
    if (listen(serverSocket, 40) == -1)
    {
        cerr << "Error: Could not listen on port " << port << "." << endl;
        close(serverSocket);
        return;
    }

    cout << "Server listening on port " << port << "...🚀" << endl;

    while (true)
    {
        // Accept a client connection
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddrLen);
        if (clientSocket == -1)
        {
            cerr << "Error: Could not accept client connection." << endl;
            continue;
        }
        createLogEntry("New client connected ");
        cout << "New client connected. IP address: " << inet_ntoa(clientAddress.sin_addr) << endl;
        cout << "Port:" << clientAddress.sin_port << endl;

        thread clientThread(handleClientRequest, clientSocket, port);
        clientThread.detach(); // Detach the thread to run independently
    }

    // Close the server socket (this part of the code is unreachable in this example)
    close(serverSocket);
    cout << "reaching";
}

// char input[100];
// int bytesRead;

// bytesRead = read(port, input, sizeof(input));
// input[bytesRead] = '\0';
// string filePath(input);
// filePath = "./" + filePath;
// char *fp = new char[filePath.length() + 1];
// strcpy(fp, filePath.c_str());
// cout << "sha: " << getFileHash(fp) << endl;
// // Open the file using open system call
// int fileDescriptor = open(filePath.c_str(), O_RDONLY);
// if (fileDescriptor != -1)
// {
//     // Get the size of the file
//     struct stat fileStat;
//     fstat(fileDescriptor, &fileStat);
//     size_t fileSize = fileStat.st_size;

//     // Define buffer size for reading and sending data
// const int bufferSize = 524288; // 512 KB chunks
// char *buffer = new char[bufferSize];

//     // Read and send file in chunks
//     ssize_t totalBytesSent = 0;
//     while (totalBytesSent < fileSize)
//     {
//         // Calculate the remaining bytes to send
//         size_t remainingBytes = fileSize - totalBytesSent;
//         // Determine the actual size of the current chunk
//         size_t chunkSize = (remainingBytes < bufferSize) ? remainingBytes : bufferSize;

//         // Read a chunk of data from the file
//         ssize_t bytesRead = read(fileDescriptor, buffer, chunkSize);

//         if (bytesRead <= 0)
//         {
//             // Error reading file, handle it here (break or return or clean up resources)
//             cout << "Error reading file.";
//             break;
//         }

//         // Send the chunk of data to the client
//         ssize_t bytesSent = send(port, buffer, bytesRead, 0);

//         if (bytesSent <= 0)
//         {
//             // Error sending data to client, handle it here (break or return or clean up resources)
//             cout << "Error sending data to client.";
//             break;
//         }

//         // Update total bytes sent
//         totalBytesSent += bytesSent;
//         cout << totalBytesSent << endl;
//     }

//     // Close the file descriptor
//     close(fileDescriptor);
// }
// else
// {
//     // If the file cannot be opened, send an error message to the client
//     string errorMessage = "Error: File not found or cannot be opened.";
//     send(port, errorMessage.c_str(), errorMessage.size(), 0);
//     cout << "Error: File not found or cannot be opened." << endl;
// }

// close(port);
// cout << "Connection closed." << endl;