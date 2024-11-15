#include "headers.h"

mutex chunkInfoMutex;
class ThreadPool
{
public:
    ThreadPool(int numThreads) : stop(false)
    {
        for (int i = 0; i < numThreads; ++i)
        {
            workers.emplace_back([this]
                                 {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) {
                            return;
                        }
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                } });
        }
    }

    // Add a task to the thread pool
    template <class F>
    void enqueue(F &&f)
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one();
    }

    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers)
        {
            worker.join();
        }
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

void printFileDetails(const std::unordered_map<std::string, FileDetails> &files)
{
    std::cout << "-----------------client--------------------" << std::endl;
    for (const auto &file : files)
    {
        std::cout << "File Name: " << file.second.fileName << std::endl;
        std::cout << "File Path: " << file.second.filePath << std::endl;
        std::cout << "File Size: " << file.second.fileSize << std::endl;
        std::cout << "Number of Chunks: " << file.second.numberOfChunks << std::endl;

        std::cout << "Chunks Hashes:" << std::endl;
        for (const auto &chunkHash : file.second.chunksHash)
        {
            std::cout << "Chunk Number: " << chunkHash.first << " Hash: " << chunkHash.second << std::endl;
        }

        std::cout << "File Hash: " << file.second.fileHash << std::endl;
        cout << "GroupId: " << file.second.groupId << endl;
        std::cout << "--------------------------------------" << std::endl;
    }
}

void printChunkInfo(const std::unordered_map<std::string, std::unordered_map<long long, std::vector<std::string>>> &chunkInfo)
{
    for (const auto &entry : chunkInfo)
    {
        std::cout << "Key: " << entry.first << std::endl;
        const auto &innerMap = entry.second;

        for (const auto &innerEntry : innerMap)
        {
            std::cout << "  Sub-Key: " << innerEntry.first << std::endl;
            const auto &vectorData = innerEntry.second;

            std::cout << "  Values: ";
            for (const auto &value : vectorData)
            {
                std::cout << value << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
}

void pingTracker(int trackerSocket, pair<string, string> socketInfo)
{
    createLogEntry("Connection closed Abruptly");
    cout << "Connection closed Abruptly!!🥹" << endl;
    trackerSocket = -1;

    while (trackerSocket < 0)
    {

        trackerSocket = createConnection(socketInfo);
        if (trackerSocket < 0)
        {
            cout << "Connection failed!!🥲" << endl;
            trackerSocket = -1;
        }
        else
        {
            cout << "Hurray! Connection establised, Enter Previous Command Again!!!😊" << endl;
        }
        sleep(3);
    }

    return;
}

unordered_map<string, FileDownloadInfo> parseDownloadReply(string input, string destinationPath, int groupId)
{
    istringstream iss(input);
    string fileName, sha, fileSize, peerSocket;
    iss >> fileName >> sha >> fileSize;

    long long size = stoll(fileSize);
    vector<string> peerSockets;
    while (iss >> peerSocket)
    {
        peerSockets.push_back(peerSocket);
    }

    unordered_map<string, FileDownloadInfo> fileDownloadInfo;
    FileDownloadInfo fileInfo = {sha, peerSockets, size};
    fileDownloadInfo[fileName] = fileInfo;
    pendingDownloads.push(fileDownloadInfo);
    shaToFileNameMapping[sha] = fileName;

    FileDetails fileDetails;
    fileDetails.fileName = fileName;
    fileDetails.filePath = destinationPath;
    fileDetails.fileSize = size;
    fileDetails.groupId = groupId;
    fileDetails.fileHash = sha;
    fileDetails.numberOfChunks = ceil((float)size / 524288);
    files[fileName] = fileDetails;
    // auto front = pendingDownloads.front();
    // for (const auto &pair : front)
    // {
    //     cout << "File Name: " << pair.first << endl;
    //     cout << "SHA: " << pair.second.sha << endl;
    //     cout << "Size: " << pair.second.size << endl;
    //     cout << "Peer Sockets: ";
    //     for (const auto &socket : pair.second.peerSockets)
    //     {
    //         cout << socket << " ";
    //     }
    //     cout << endl;
    // }
    return fileDownloadInfo;
}

void downloadFromPeer(string socket, string fileSha, unordered_map<string, unordered_map<long long, vector<string>>> &chunkInfo)
{
    const int bufferSize = 32768; // 512 KB chunks
    char *buffer = new char[bufferSize];
    // char buffer[512];
    pair<string, string> socketInfo = getIpAndPortFromSocketInfo(socket);
    int peerSocket = createConnection(socketInfo);

    if (peerSocket < 0)
    {
        createLogEntry("Unable to connect with peer");
        cerr << "Unable to connect with peer !!" << endl;
    }
    string message = "GET_AVAILABLE_CHUNK_INFO " + fileSha;
    strcpy(buffer, message.c_str());

    write(peerSocket, buffer, bufferSize);
    if (read(peerSocket, buffer, bufferSize) == 0)
    {
        cout << "error";
    }
    string response(buffer);

    vector<string> responseVector = tokenizeCommand(response, ' ');
    if (responseVector[0] == "error")
    {
        cout << "Cannot find file!!";
        return;
    }
    // cout << "response Vec:" << endl;
    // for (int i = 0; i < responseVector.size(); i++)
    // {
    //     cout << responseVector[i] << endl;
    // }
    // cout << "end";
    string availableChunks = responseVector.back();
    istringstream inputStream(availableChunks);
    string token;
    while (std::getline(inputStream, token, '&'))
    {
        // Convert token to socket number
        long long socket = std::stoll(token);
        std::unique_lock<std::mutex> lock(chunkInfoMutex);
        // Update or create entry in the unordered_map with the socket as the key
        chunkInfo[fileSha][socket].push_back(responseVector[0]); // Insert socket as the first element in the vector
        lock.unlock();
    }
}

void getChunkDetailsInfoFromPeers(vector<string> sockets, string fileSha, unordered_map<string, unordered_map<long long, vector<string>>> &chunkInfo)
{
    unordered_map<string, unordered_map<long long, vector<string>>> chunkInfoDup = chunkInfo;
    ThreadPool pool(10);
    for (const std::string &peer : sockets)
    {
        pool.enqueue([peer, fileSha, &chunkInfo]
                     { downloadFromPeer(peer, fileSha, chunkInfo); });
    }
}

void makeRequest(pair<string, string> socketInfo, string serverSocket)
{
    int trackerSocket;
    unordered_map<string, unordered_map<long long, vector<string>>> chunkInfo;

    if ((trackerSocket = createConnection(socketInfo)) < 0)
    {
        createLogEntry("Connection with Tracker 0 failed...🥹");
    }
    char buffer[1024];
    // const int bufferSize = 524288; // 512 KB chunks
    // char *buffer = new char[bufferSize];
    string userCommand;
    while (running)
    {
        buffer[0] = '\0';
        // cin.ignore();
        getline(cin, userCommand);
        vector<string> tokens = tokenizeCommand(userCommand, ' ');

        if (tokens[0] == "create_user")
        {
            if (tokens.size() != 3)
            {
                cout << "Invalid command!!❌" << endl;
                continue;
            }
            string userId = tokens[1];
            string password = tokens[2];
            char delimiter = '\x1C';
            string message = tokens[0] + delimiter + userId + delimiter + password;
            strcpy(buffer, message.c_str());
            write(trackerSocket, buffer, sizeof(buffer));

            if (read(trackerSocket, buffer, sizeof(buffer)) == 0)
            {
                pingTracker(trackerSocket, socketInfo);
                continue;
            }
            cout << buffer << endl;
            createLogEntry(buffer);
        }
        if (tokens[0] == "login")
        {
            if (tokens.size() != 3)
            {
                cout << "Invalid command!!❌" << endl;
                continue;
            }
            string userId = tokens[1];
            string password = tokens[2];
            char delimiter = '\x1C';
            string message = tokens[0] + delimiter + userId + delimiter + password + delimiter + serverSocket;
            strcpy(buffer, message.c_str());
            write(trackerSocket, buffer, sizeof(buffer));

            if (read(trackerSocket, buffer, sizeof(buffer)) == 0)
            {
                pingTracker(trackerSocket, socketInfo);
                continue;
            }
            cout << buffer << endl;
            createLogEntry(buffer);
        }
        if (tokens[0] == "logout")
        {
            if (tokens.size() != 1)
            {
                cout << "Invalid command!!❌" << endl;
                continue;
            }
            string message = tokens[0];
            strcpy(buffer, message.c_str());
            write(trackerSocket, buffer, sizeof(buffer));

            if (read(trackerSocket, buffer, sizeof(buffer)) == 0)
            {
                pingTracker(trackerSocket, socketInfo);
                continue;
            }
            cout << buffer << endl;
            createLogEntry(buffer);
        }
        if (tokens[0] == "create_group")
        {
            if (tokens.size() != 2)
            {
                cout << "Invalid command!!❌" << endl;
                continue;
            }
            char delimiter = '\x1C';
            string message = tokens[0] + delimiter + tokens[1];
            strcpy(buffer, message.c_str());
            write(trackerSocket, buffer, sizeof(buffer));

            if (read(trackerSocket, buffer, sizeof(buffer)) == 0)
            {
                pingTracker(trackerSocket, socketInfo);
                continue;
            }
            cout << buffer << endl;
            createLogEntry(buffer);
        }
        if (tokens[0] == "join_group")
        {
            if (tokens.size() != 2)
            {
                cout << "Invalid command!!❌" << endl;
                continue;
            }
            char delimiter = '\x1C';
            string message = tokens[0] + delimiter + tokens[1];
            strcpy(buffer, message.c_str());
            write(trackerSocket, buffer, sizeof(buffer));

            if (read(trackerSocket, buffer, sizeof(buffer)) == 0)
            {
                pingTracker(trackerSocket, socketInfo);
                continue;
            }
            cout << buffer << endl;
            createLogEntry(buffer);
        }
        if (tokens[0] == "show_downloads")
        {
            if (tokens.size() != 1)
            {
                cout << "Invalid command!!❌" << endl;
                continue;
            }
            for (const auto &filePair : files)
            {
                const FileDetails &file = filePair.second;

                // Check if chunksHash is empty
                if (file.chunksHash.empty())
                {
                    cout << "[D] [" << file.groupId << "] " << file.fileName << endl;
                }
                else
                {
                    cout << "[C] [" << file.groupId << "] " << file.fileName << endl;
                }
            }
        }
        if (tokens[0] == "leave_group")
        {
            if (tokens.size() != 2)
            {
                cout << "Invalid command!!❌" << endl;
                continue;
            }
            char delimiter = '\x1C';
            string message = tokens[0] + delimiter + tokens[1];
            strcpy(buffer, message.c_str());
            write(trackerSocket, buffer, sizeof(buffer));

            if (read(trackerSocket, buffer, sizeof(buffer)) == 0)
            {
                pingTracker(trackerSocket, socketInfo);
                continue;
            }
            cout << buffer << endl;
            createLogEntry(buffer);
        }
        if (tokens[0] == "stop_share")
        {
            if (tokens.size() != 3)
            {
                cout << "Invalid command!!❌" << endl;
                continue;
            }
            char delimiter = '\x1C';
            string message = tokens[0] + delimiter + tokens[1] + delimiter + tokens[2];
            strcpy(buffer, message.c_str());
            write(trackerSocket, buffer, sizeof(buffer));

            if (read(trackerSocket, buffer, sizeof(buffer)) == 0)
            {
                pingTracker(trackerSocket, socketInfo);
                continue;
            }
            cout << buffer << endl;
            createLogEntry(buffer);
        }
        if (tokens[0] == "list_requests")
        {
            if (tokens.size() != 2)
            {
                cout << "Invalid command!!❌" << endl;
                continue;
            }
            char delimiter = '\x1C';
            string message = tokens[0] + delimiter + tokens[1];
            strcpy(buffer, message.c_str());
            write(trackerSocket, buffer, sizeof(buffer));

            if (read(trackerSocket, buffer, sizeof(buffer)) == 0)
            {
                pingTracker(trackerSocket, socketInfo);
                continue;
            }
            string response(buffer);
            vector<string> tokenizeResponse = tokenizeCommand(response, ':');
            if (tokenizeResponse.size() < 2)
            {
                cout << buffer << endl;
                continue;
            }
            vector<string> requestList = tokenizeCommand(tokenizeResponse[1], ' ');
            cout << "----------------Pending Requests--------------------" << endl;
            for (int i = 0; i < requestList.size(); i++)
            {
                cout << requestList[i] << endl;
            }
            createLogEntry(buffer);
        }
        if (tokens[0] == "accept_request")
        {
            if (tokens.size() != 3)
            {
                cout << "Invalid command!!❌" << endl;
                continue;
            }
            char delimiter = '\x1C';
            string message = tokens[0] + delimiter + tokens[1] + delimiter + tokens[2];
            strcpy(buffer, message.c_str());
            write(trackerSocket, buffer, sizeof(buffer));

            if (read(trackerSocket, buffer, sizeof(buffer)) == 0)
            {
                pingTracker(trackerSocket, socketInfo);
                continue;
            }
            cout << buffer << endl;
            createLogEntry(buffer);
        }
        if (tokens[0] == "list_groups")
        {
            if (tokens.size() != 1)
            {
                cout << "Invalid command!!❌" << endl;
                continue;
            }

            string message = tokens[0];
            strcpy(buffer, message.c_str());
            write(trackerSocket, buffer, sizeof(buffer));

            if (read(trackerSocket, buffer, sizeof(buffer)) == 0)
            {
                pingTracker(trackerSocket, socketInfo);
                continue;
            }
            string response(buffer);
            vector<string> tokenizeResponse = tokenizeCommand(response, ':');
            if (tokenizeResponse.size() < 2)
            {
                cout << buffer << endl;
                continue;
            }
            vector<string> grouplist = tokenizeCommand(tokenizeResponse[1], ' ');
            cout << "----------------Groups List--------------------" << endl;
            for (int i = 0; i < grouplist.size(); i++)
            {
                cout << grouplist[i] << endl;
            }
            createLogEntry(buffer);
        }
        if (tokens[0] == "list_files")
        {
            if (tokens.size() != 2)
            {
                cout << "Invalid command!!❌" << endl;
                continue;
            }
            char delimiter = '\x1C';
            string message = tokens[0] + delimiter + tokens[1];
            strcpy(buffer, message.c_str());
            write(trackerSocket, buffer, sizeof(buffer));

            if (read(trackerSocket, buffer, sizeof(buffer)) == 0)
            {
                pingTracker(trackerSocket, socketInfo);
                continue;
            }
            string response(buffer);
            vector<string> tokenizeResponse = tokenizeCommand(response, ':');
            if (tokenizeResponse.size() < 2)
            {
                cout << buffer << endl;
                continue;
            }
            vector<string> grouplist = tokenizeCommand(tokenizeResponse[1], ' ');
            cout << "----------------Files List--------------------" << endl;
            for (int i = 0; i < grouplist.size(); i++)
            {
                cout << grouplist[i] << endl;
            }
            createLogEntry(buffer);
        }
        if (tokens[0] == "upload_file")
        {
            if (tokens.size() != 3)
            {
                cout << "Invalid command!!❌" << endl;
                continue;
            }
            string filePath = tokens[1];
            char *fp = new char[filePath.length() + 1];
            strcpy(fp, filePath.c_str());
            string groupId = tokens[2];
            string fileSha = getFileHash(fp);
            long long fileSize = getFileSize(filePath);
            string fileName = filePath;
            size_t found = filePath.find_last_of('/');

            if (found != string::npos)
            {
                fileName = filePath.substr(found + 1);
            }

            char delimiter = '\x1C';
            string message = "";
            for (int i = 0; i < tokens.size(); i++)
            {
                message += tokens[i];
                if (i < tokens.size() - 1)
                {
                    message += delimiter;
                }
            }

            message = (message + delimiter + fileSha + delimiter + to_string(fileSize));
            strcpy(buffer, message.c_str());

            write(trackerSocket, buffer, sizeof(buffer));

            if (read(trackerSocket, buffer, sizeof(buffer)) == 0)
            {
                pingTracker(trackerSocket, socketInfo);
                continue;
            }

            string response(buffer);
            vector<string> responseVector = tokenizeCommand(response, ':');

            if (responseVector[0] == "200")
            {
                unordered_map<long long, string> chunksHash = calculateSHAOfAllChunks(filePath);
                FileDetails fileDetails;
                fileDetails.fileName = fileName; // Assign the actual file name here
                fileDetails.filePath = filePath;
                fileDetails.fileSize = fileSize;
                fileDetails.chunksHash = chunksHash; // You can populate chunksHash if needed
                fileDetails.fileHash = fileSha;
                fileDetails.numberOfChunks = getTotalChunksOfFile(filePath); //
                fileDetails.groupId = stoi(groupId);
                files[fileName] = fileDetails;
                shaToFileNameMapping[fileSha] = fileName;
            }
            cout << buffer << endl;
            // printFileDetails(files);
        }
        if (tokens[0] == "download_file")
        {
            if (tokens.size() != 4)
            {
                cout << "Invalid command!!❌" << endl;
                continue;
            }
            string filePath = tokens[2];
            char *fp = new char[filePath.length() + 1];
            strcpy(fp, filePath.c_str());
            string groupId = tokens[1];
            string destinationPath = tokens[3];
            string fileName = filePath;
            size_t found = filePath.find_last_of('/');
            if (found != string::npos)
            {
                fileName = filePath.substr(found + 1);
            }

            char delimiter = '\x1C';
            string message = "";
            for (int i = 0; i < tokens.size(); i++)
            {
                message += tokens[i];
                if (i < tokens.size() - 1)
                {
                    message += delimiter;
                }
            }
            strcpy(buffer, message.c_str());

            write(trackerSocket, buffer, sizeof(buffer));

            if (read(trackerSocket, buffer, sizeof(buffer)) == 0)
            {
                pingTracker(trackerSocket, socketInfo);
                continue;
            }

            string response(buffer);
            vector<string> responseVector = tokenizeCommand(response, '-');
            if (responseVector[0] != "200")
            {
                cout << "Some issue with download enter command again!!😞";
                createLogEntry("Some issue with download enter command again!!");
                continue;
            }
            unordered_map<string, FileDownloadInfo> fileDownloadInfo = parseDownloadReply(responseVector[1], destinationPath, stoi(groupId));
            getChunkDetailsInfoFromPeers(fileDownloadInfo[fileName].peerSockets, fileDownloadInfo[fileName].sha, chunkInfo);
            // printChunkInfo(chunkInfo);
            createLogEntry("control enter to downloadchunk");

            downloadChunks(destinationPath, fileName, fileDownloadInfo[fileName].sha, fileDownloadInfo[fileName].size, chunkInfo);
            cout
                << buffer << endl;
        }
    }
}
