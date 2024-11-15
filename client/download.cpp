#include "headers.h"

// Mutex for file operations
std::mutex bufferMutex; // Mutex for buffer operations

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

    void waitAll()
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        condition.wait(lock, [this]
                       { return tasks.empty(); });
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

void downloadFile(int peerSocket, string fileName)
{
    char *receive_buffer = new char[524288]; // Buffer for receiving chunks
    string s = "../received/" + fileName;
    const char *filePath = s.c_str();

    // Open the file for writing
    int fileDescriptor = open(filePath, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fileDescriptor < 0)
    {
        cerr << "Error: Unable to open file for writing." << endl;
        close(peerSocket);
        delete[] receive_buffer;
        return;
    }

    while (true)
    {
        // Receive a chunk of data (maximum of 512 bytes)
        int bytesRead = read(peerSocket, receive_buffer, 524288);
        if (bytesRead <= 0)
        {
            // Error occurred or end of file
            break;
        }

        // Write the received chunk to the file
        int bytesWritten = write(fileDescriptor, receive_buffer, bytesRead);
        if (bytesWritten < bytesRead)
        {
            cerr << "Error occurred while writing to file." << endl;
            break;
        }
    }

    // Close the file descriptor and the socket connection
    close(fileDescriptor);
    close(peerSocket);
    delete[] receive_buffer;
}

bool compareVectors(const pair<long long, vector<string>> &a, const pair<long long, vector<string>> &b)
{
    return a.second.size() < b.second.size();
}

// void piceWiseAlgorithm()
// {

//     vector<pair<long long, vector<string>>> pairs(chunkInfo.begin(), chunkInfo.end());

//     sort(pairs.begin(), pairs.end(), compareVectors);

//     chunkInfo.clear();
//     for (const auto &pair : pairs)
//     {
//         chunkInfo[pair.first] = pair.second;
//     }
// }

void downloadFileFromPeer(vector<string> peers, long long chunkNo, string destinationPath, string filePath, string fileSha, long long size)
{
    srand(static_cast<unsigned long long>(time(0)));
    long long randomIndex = rand() % peers.size();
    string randomPeer = peers[randomIndex];
    string fileName = filePath.substr(filePath.find_last_of('/') + 1);
    // int fileDescriptor = open(destinationPath.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);

    string s = destinationPath;

    // Open the file for writing
    std::unique_lock<std::mutex> fileLock(fileMutex);

    // int fileDescriptor = open(s.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    int fileDescriptor = open(s.c_str(), O_RDWR | O_CREAT, S_IRWXU);

    if (fileDescriptor < 0)
    {
        cerr << "Error: Unable to open file for writing." << endl;

        fileLock.unlock(); // Release the file lock before returning
        return;
    }
    pair<string, string> ipAndPort = getIpAndPortFromSocketInfo(randomPeer);
    int peerSocket = createConnection(ipAndPort);
    string message = "GIVE_CHUNK " + fileSha + " " + fileName + " " + to_string(chunkNo);
    char buffer[1024];
    strcpy(buffer, message.c_str());
    send(peerSocket, buffer, sizeof(buffer), 0);
    memset(buffer, 0, sizeof(buffer));
    read(peerSocket, buffer, 1024);
    string responseHash(buffer);
    cout << responseHash << endl;

    vector<string> tokenizeHash = tokenizeCommand(responseHash, ':');
    string chunkHash = "";
    if (tokenizeHash[0] == "hash")
    {
        chunkHash = tokenizeHash[1];
    }
    if (files.find(fileName) != files.end())
    {
        // FileKey exists, add the chunkHash to chunksHash map
        files[fileName].chunksHash[chunkNo] = chunkHash;
        cout << "Chunk added successfully. File: " << files[fileName].fileName
             << ", Chunk No: " << chunkNo << ", Chunk Hash: " << chunkHash << endl;
    }
    string respond = "received";
    buffer[0] = '\0';
    strcpy(buffer, respond.c_str());
    send(peerSocket, buffer, sizeof(buffer), 0);
    std::unique_lock<std::mutex>
        bufferLock(bufferMutex);
    // cout << "size:" << size << endl;
    long long readBufferSize = (524288 < (size - (524288 * chunkNo))) ? 524288 : (size - (524288 * chunkNo));
    // cout << "readBufferSize" << readBufferSize << endl;
    char *receive_buffer = new char[readBufferSize]; // Buffer for receiving chunks
    int readByte, bytesSent;
    // off_t offset = 524288 * chunkNo;

    long totalBytesReceived = 0;
    long bytesReceived;

    // memset(receive_buffer, 0, strlen(receive_buffer));

    bzero(receive_buffer, readBufferSize);
    // if ((readByte = read(peerSocket, receive_buffer, readBufferSize)) <= 0)
    // {
    //     createLogEntry("Read fail : " + fileSha + " " + to_string(chunkNo) + " ");
    //     cout << "Read fail : " << fileSha << " " << chunkNo << " " << endl;
    //     close(fileDescriptor);
    //     delete[] receive_buffer;
    //     bufferLock.unlock();
    //     fileLock.unlock();
    //     return;
    // }
    // cout << "readByte" << readByte << endl;
    // createLogEntry("Read successful");
    // int bytesWritten = pwrite(fileDescriptor, receive_buffer, readBufferSize, 524288 * chunkNo);
    // cout << "receiver side byte read:" << bytesWritten << " " << chunkNo << endl;
    ssize_t bytesRead = 0;

    // Fill receive_buffer with data received in chunks
    while (totalBytesReceived < readBufferSize)
    {
        // Read 32KB chunks from the peer
        int chunkSize = 32768; // 32KB chunk size
        if (readBufferSize - totalBytesReceived < chunkSize)
        {
            chunkSize = readBufferSize - totalBytesReceived;
        }

        bytesRead = read(peerSocket, receive_buffer + totalBytesReceived, chunkSize);
        if (bytesRead <= 0)
        {
            createLogEntry("Read fail: " + fileSha + " " + to_string(chunkNo));
            cerr << "Read fail: " << fileSha << " " << chunkNo << endl;
            close(fileDescriptor);
            delete[] receive_buffer;
            bufferLock.unlock();
            fileLock.unlock();
            return;
        }

        totalBytesReceived += bytesRead;
    }
    // delete[] receive_buffer;
    off_t offset = 524288 * chunkNo; // Calculate the offset for pwrite
    string receivedChunkHash = getChunkHash(receive_buffer, totalBytesReceived);
    // cout << "received:" << receivedChunkHash << endl;
    ssize_t bytesWritten = pwrite(fileDescriptor, receive_buffer, totalBytesReceived, offset);
    if (bytesWritten != totalBytesReceived)
    {
        cerr << "Error: Unable to write data to file." << endl;
    }
    else
    {
        createLogEntry("Read successful");
        cout << "Successfully received and wrote " << totalBytesReceived << " bytes to file." << endl;
    }

    // Write the entire receive_buffer to the file in a single shot
    // ssize_t bytesWritten = write(fileDescriptor, receive_buffer, totalBytesReceived);
    // if (bytesWritten != totalBytesReceived)
    // {
    //     cerr << "Error: Unable to write data to file." << endl;
    // }
    // else
    // {
    //     createLogEntry("Read successful");
    //     cout << "Successfully received and wrote " << totalBytesReceived << " bytes to file." << endl;
    // }

    bufferLock.unlock();
    close(peerSocket);
    close(fileDescriptor);
    fileLock.unlock();
    delete[] receive_buffer;

    // bufferLock.unlock();

    // if (bytesWritten < 0)
    // {
    //     cerr << "Error: Unable to write data to file." << endl;
    // }
    // else
    // {
    //     cout << "Successfully received and wrote " << bytesWritten << " bytes to file." << endl;
    // }

    // close(peerSocket);

    // close(fileDescriptor);
    // // fileLock.unlock();
    // delete[] receive_buffer;
}

// void downloadChunks(string destinationPath, string filePath, string fileSha, long long size)
// {

//     for (const auto &pair : chunkInfo)
//     {
//         const long long chunkNo = pair.first;
//         const vector<string> peers = pair.second;

//         // Enqueue tasks to download chunks from peers
//         downloadFileFromPeer(peers, chunkNo, destinationPath, filePath, fileSha, size);
//         std::this_thread::sleep_for(std::chrono::milliseconds(500));
//     }

//     // pool.waitAll();
//     cout << "Download complete. Chunks saved to: " << destinationPath << endl;
//     return;
// }

void downloadChunks(string destinationPath, string filePath, string fileSha, long long size, unordered_map<string, unordered_map<long long, vector<string>>> &chunkInfo)
{

    ThreadPool pool(10);
    unordered_map<long long, vector<string>> &peerChunk = chunkInfo[fileSha];
    for (const auto &pair : peerChunk)
    {
        const long long chunkNo = pair.first;
        const vector<string> peers = pair.second;

        // Enqueue tasks to download chunks from peers
        createLogEntry(to_string(chunkNo) + " is downloading");
        pool.enqueue([peers, chunkNo, destinationPath, filePath, fileSha, size]
                     { downloadFileFromPeer(peers, chunkNo, destinationPath, filePath, fileSha, size); });
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // pool.waitAll();
    cout << "Download complete. Chunks saved to: " << destinationPath << endl;
    return;
}

// bytesReceived = recv(peerSocket, receive_buffer, readBufferSize, 0);
// if (bytesReceived <= 0)
// {
//     std::cerr << "Error receiving data from server\n";
// }
// lseek(fileDescriptor, 0, SEEK_SET); // making pointer to zero

// lseek(fileDescriptor, 524288 * chunkNo, SEEK_SET);
// size_t lsize;
// unsigned long long int mark = 18446744073709551615;
// while ((lsize = recv(peerSocket, receive_buffer, 65000, 0)) > 0)
// {
//     cout << lsize << endl;
//     if (lsize == mark)
//         break;
//     cout << "receiving" << endl;
//     long long int k = write(fileDescriptor, receive_buffer, lsize);
//     // fwrite(buff, 1, size, dest);
//     memset(receive_buffer, 0, readBufferSize);
//     // mlog("\nreceive_file: size read " + to_string(size) + " " + to_string(k) + "\n");
// }
// cout << "comppeted" << endl;
// cout << "bytesReceive: " << bytesReceived << endl;
// ssize_t bytesWritten = pwrite(fileDescriptor, receive_buffer, bytesReceived, 524288 * chunkNo);
// if (bytesWritten == -1)
// {
//     std::cerr << "Error writing data to output file\n";
// }
// else
// {
//     std::cout << "Written " << bytesWritten << " bytes to outputfile.bin at position " << 524288 * chunkNo << "\n";
// }
