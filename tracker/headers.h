#ifndef HEADERS_H
#define HEADERS_H
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <bits/stdc++.h>
#include <signal.h>
#include <semaphore.h>
#include <bits/stdc++.h>
using namespace std;
void createLogEntry(string logMessage);
pair<string, string> getIpAndPortFromSocketInfo(string socketInfo);
vector<pair<string, string>> readAddress(char *fileName);
int createConnection(pair<string, string> socketInfo);
void startTracker(int port);
void createLogEntry(string logMessage);
vector<string> tokenizeString(const string &inputString);

extern bool running;
extern unordered_map<string, string> shaToFileNameMapping;
struct UserDetails
{
    string password;
    bool isLoggedIn;
    string socket;
};
extern unordered_map<string, UserDetails> users;

struct FileDetails
{
    string sha;
    long long size;
    vector<string> peersSocket;
};

struct Group
{
    string adminUserId;
    unordered_set<string> joinRequests;
    unordered_set<string> members;
    unordered_map<string, FileDetails> files;
};
extern unordered_map<int, Group> groupDetails;
#endif