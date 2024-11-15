#ifndef HEADERS_H
#define HEADERS_H
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include <sys/mman.h>
#include <unistd.h>
#include <thread>
#include <iostream>
#include <sstream>
#include <bits/stdc++.h>
#include <utility>
#include <unistd.h>
#include <semaphore.h>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <chrono>

using namespace std;

vector<string> getHash(char *path);
string getFileHash(char *path);
long long file_size(char *);
void getStringHash(string segmentString, vector<string> &hash);
void startServer(int port);
void createLogFile();
void createLogEntry(string logMessage);
pair<string, string> getIpAndPortFromSocketInfo(string socketInfo);
vector<pair<string, string>> readAddress(char *fileName);
int createConnection(pair<string, string> socketInfo);
void downloadFile(int peerSocket, string fileName);
void makeRequest(pair<string, string> socketInfo, string serverSocket);
vector<string> tokenizeCommand(string command, char delimiter);
long long getFileSize(const string &filename);
long long getTotalChunksOfFile(const string &filename);
unordered_map<long long, string> calculateSHAOfAllChunks(string &filename);
void downloadChunks(string destinationPath, string fileName, string fileSha, long long size, unordered_map<string, unordered_map<long long, vector<string>>> &chunkInfo);
string getChunkHash(char *buffer, long bytesRead);

extern bool running;
struct FileDetails
{
    string fileName;
    string filePath;
    long long fileSize;
    unordered_map<long long, string> chunksHash;
    string fileHash;
    long long numberOfChunks;
    int groupId;
};
extern unordered_map<string, FileDetails> files;
struct FileDownloadInfo
{
    string sha;
    vector<string> peerSockets;
    long long size;
};

extern unordered_map<string, string> shaToFileNameMapping;
// extern unordered_map<long long, vector<string>> chunkInfo;
extern queue<unordered_map<string, FileDownloadInfo>> pendingDownloads;
extern queue<unordered_map<string, FileDownloadInfo>> compeletedDownloads;
extern std::mutex fileMutex;
#endif
