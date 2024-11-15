#include "headers.h"

// void addStaticUsers()
// {
//     // Create and populate static UserDetails objects
//     string password1 = "password123";
//     string socket1 = "127.0.0.1:6000";
//     // UserDetails user1(password1, true, socket1);
//     UserDetails user1;
//     user1.password = password1;
//     user1.isLoggedIn = true;
//     user1.socket = socket1;
//     users["user1"] = user1;

//     // Create and populate static Group objects
//     Group group1;
//     group1.adminUserId = "user1";
//     group1.members.insert("user1");

//     // Add the static Group objects to the groupDetails unordered_map
//     groupDetails[1] = group1;
// }

void printUserDetails(const unordered_map<string, UserDetails> &users)
{
    // Iterate through the unordered_map and print UserDetails for each user
    for (const auto &user : users)
    {
        cout << "User ID: " << user.first << endl;
        cout << "Password: " << user.second.password << endl;
        cout << "Is Logged In: " << (user.second.isLoggedIn ? "true" : "false") << endl;
        cout << "Socket: " << user.second.socket << endl;
        cout << "---------------------" << endl;
    }
}

void printGroupDetails(const unordered_map<int, Group> &groups)
{

    for (const auto &groupPair : groups)
    {
        int groupId = groupPair.first;
        const Group &group = groupPair.second;

        cout << "Group ID: " << groupId << endl;
        cout << "Admin User ID: " << group.adminUserId << endl;
        cout << "Join Requests: ";
        for (const auto &joinRequest : group.joinRequests)
        {
            cout << joinRequest << " ";
        }
        cout << endl;

        cout << "Members: ";
        for (const auto &member : group.members)
        {
            cout << member << " ";
        }
        cout << endl;

        cout << "Files:" << endl;
        for (const auto &filePair : group.files)
        {
            const string &fileName = filePair.first;
            const FileDetails &fileDetails = filePair.second;

            cout << "  File Name: " << fileName << endl;
            cout << "  SHA: " << fileDetails.sha << endl;
            cout << "  Size: " << fileDetails.size << endl;

            cout << "  Peer Sockets: ";
            for (const auto &peerSocket : fileDetails.peersSocket)
            {
                cout << peerSocket << " ";
            }
            cout << endl;
        }
        cout << "------------------------" << endl;
    }
}

bool isUserLoggedIn(const string &userId)
{
    auto userIterator = users.find(userId);

    if (userIterator != users.end())
    {
        return userIterator->second.isLoggedIn;
    }
    return false;
}

bool isUserInGroup(int groupId, const string &currUserId)
{
    auto it = groupDetails.find(groupId);
    if (it != groupDetails.end())
    {
        const unordered_set<string> &membersSet = it->second.members;
        return membersSet.find(currUserId) != membersSet.end();
    }
    return false; // Group with group_id not found
}

void handleClientRequest(int clientPort, sockaddr_in clientAddress)
{
    char buffer[1024];
    string currUserId = "";
    int login = true;
    string connectport;
    bool clientUp = true;

    while (clientUp)
    {
        buffer[0] = '\0';
        int bytesRead = read(clientPort, buffer, sizeof(buffer));
        if (bytesRead == 0)
        {
            cout << "Client Terminated!!" << endl;
            clientUp = false;
            continue;
        }
        string clientRequest(buffer);
        vector<string> tokens = tokenizeString(clientRequest);
        if (tokens[0] == "download_file")
        {
            // printGroupDetails(groupDetails);

            string filePath = tokens[2];
            int groupId = stoi(tokens[1]);
            string fileName = filePath;
            size_t found = filePath.find_last_of('/');

            if (found != string::npos)
            {
                fileName = filePath.substr(found + 1);
            }
            auto groupIt = groupDetails.find(groupId);

            if (!isUserLoggedIn(currUserId))
            {
                string message = "You are not logged in!!👎";
                createLogEntry("User with id:" + currUserId + " is not login");
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
            if (groupIt == groupDetails.end())
            {
                string message = "Group with id: " + to_string(groupId) + " not found!!😞";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }

            auto &memeberSet = groupIt->second.members;
            if (memeberSet.find(currUserId) == memeberSet.end())
            {
                string message = "Only Members with Group id: " + to_string(groupId) + " are allowed to download!!🚫";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                continue;
            }

            Group &group = groupDetails[groupId];

            string message = "";
            if (group.files.find(fileName) == group.files.end())
            {
                cout << "file not there";

                string message = "File not present in the group!!🚫";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                continue;
            }
            else
            {
                string userSocket = users[currUserId].socket;
                FileDetails &fileDetails = group.files[fileName];
                fileDetails.peersSocket.push_back(userSocket);

                string sha = fileDetails.sha;
                long long size = fileDetails.size;
                string peerSockets = "";
                for (const auto &socket : fileDetails.peersSocket)
                {
                    if (socket != userSocket)
                        peerSockets += socket + " ";
                }
                if (!peerSockets.empty())
                {
                    peerSockets.pop_back();
                }

                message = "200-" + fileName + " " + sha + " " + to_string(size) + " " + peerSockets;

                createLogEntry(fileName + " details send to peer " + userSocket + " for download!!");
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
            }
        }
        if (tokens[0] == "upload_file")
        {

            string filePath = tokens[1];
            int groupId = stoi(tokens[2]);
            string fileSha = tokens[3];
            long long fileSize = stoll(tokens[4]);
            string fileName = filePath;
            size_t found = filePath.find_last_of('/');

            if (found != string::npos)
            {
                fileName = filePath.substr(found + 1);
            }

            if (!isUserLoggedIn(currUserId))
            {
                string message = "You are not logged in!!👎";
                createLogEntry("User with id:" + currUserId + " is not login");
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }

            auto groupIt = groupDetails.find(groupId);

            if (groupIt == groupDetails.end())
            {
                string message = "Group with id: " + to_string(groupId) + " not found!!😞";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                continue;
            }
            auto &memeberSet = groupIt->second.members;
            if (memeberSet.find(currUserId) == memeberSet.end())
            {
                string message = "Only Members with Group id: " + to_string(groupId) + " are allowed to upload!!🚫";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                continue;
            }

            shaToFileNameMapping[fileSha] = fileName;
            auto &group = groupDetails[groupId];
            auto fileIt = group.files.find(fileName);
            if (fileIt != group.files.end())
            {
                // File with the same filename exists, add client IP and port
                cout << "already uploaded";
                string socket = users[currUserId].socket;
                FileDetails &existingFile = fileIt->second;
                existingFile.peersSocket.push_back(socket);
            }
            else
            {

                FileDetails newFile;
                newFile.sha = fileSha;
                newFile.size = fileSize;
                string socket = users[currUserId].socket;
                newFile.peersSocket.push_back(socket);
                group.files[fileName] = newFile;
            }

            string message = "200:File info uploaded successfully!!😊";
            createLogEntry(message);
            message += "\n";
            strcpy(buffer, message.c_str());
            send(clientPort, buffer, sizeof(buffer), 0);

            // printGroupDetails(groupDetails);

            cout << buffer << endl;
        }
        if (tokens[0] == "create_user")
        {
            string userId = tokens[1];
            string password = tokens[2];

            auto it = users.find(userId);
            if (it != users.end())
            {
                // userId is present in the unordered_map
                string message = "User with userId '" + userId + " is already present.🙃!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
            }
            else
            {
                // userId is not present in the unordered_map, insert a new entry
                UserDetails newUserDetails;
                newUserDetails.password = password; // Set the password for the new user
                users.emplace(userId, newUserDetails);

                string message = "User with userId '" + userId + "' is successfully registered.🙃!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
            }
            // printUserDetails(users);
        }
        if (tokens[0] == "login")
        {

            string userId = tokens[1];
            string password = tokens[2];
            string socketInfo = tokens[3];
            auto it = users.find(userId);
            if (it == users.end())
            {
                string message = "User not exists ❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
            }
            else
            {
                UserDetails &userDetails = it->second;
                if (userDetails.isLoggedIn == true)
                {
                    string message = "User already logged in 🔐!!";
                    createLogEntry(message);
                    message += "\n";
                    strcpy(buffer, message.c_str());
                    send(clientPort, buffer, sizeof(buffer), 0);
                    cout << buffer << endl;
                }
                else if (userDetails.password != password)
                {
                    string message = "Wrong password 🔐!!";
                    createLogEntry(message);
                    message += "\n";
                    strcpy(buffer, message.c_str());
                    send(clientPort, buffer, sizeof(buffer), 0);
                    cout << buffer << endl;
                }
                else
                {
                    userDetails.isLoggedIn = true;
                    string oldSocket = userDetails.socket;
                    userDetails.socket = socketInfo;
                    currUserId = userId;
                    string message = "Hurray, Successfully logged in 🔐!!";
                    createLogEntry(message + userId);
                    message += "\n";
                    strcpy(buffer, message.c_str());
                    send(clientPort, buffer, sizeof(buffer), 0);
                    cout << buffer << endl;
                }
            }
            // printUserDetails(users);
        }
        if (tokens[0] == "logout")
        {
            if (currUserId == "")
            {
                string message = "User not logged in 🔐!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
            else
            {
                auto it = users.find(currUserId);
                if (it != users.end())
                {
                    UserDetails &userDetails = it->second;
                    if (userDetails.isLoggedIn == false)
                    {
                        string message = "User not logged in 🔐!!";
                        createLogEntry(message);
                        message += "\n";
                        strcpy(buffer, message.c_str());
                        send(clientPort, buffer, sizeof(buffer), 0);
                        cout << buffer << endl;
                        continue;
                    }
                    else
                    {
                        string message = "Succesfully Logout 🔐!!";
                        createLogEntry(message);
                        message += "\n";
                        strcpy(buffer, message.c_str());
                        send(clientPort, buffer, sizeof(buffer), 0);
                        cout << buffer << endl;
                        userDetails.isLoggedIn = false;
                        currUserId = "";
                    }
                }
                // printUserDetails(users);
            }
        }
        if (tokens[0] == "create_group")
        {
            int groupId = stoi(tokens[1]);
            if (!isUserLoggedIn(currUserId))
            {
                string message = "User is not Logged in ❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
            if (groupDetails.find(groupId) != groupDetails.end())
            {
                string message = "Group Already exists 😐!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
            else
            {
                Group newGroup;
                newGroup.adminUserId = currUserId;
                newGroup.members.insert(currUserId);
                groupDetails[groupId] = newGroup;

                string message = "Group Created Successfully 🙌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
            }
            // printGroupDetails(groupDetails);
            // printUserDetails(users);
        }
        if (tokens[0] == "stop_share")
        {
            int groupId = stoi(tokens[1]);
            string fileName = tokens[2];
            if (!isUserLoggedIn(currUserId))
            {
                string message = "User is not Logged in ❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
            if (groupDetails.find(groupId) == groupDetails.end())
            {
                string message = "Group not exists ❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }

            string peerSocket = users[currUserId].socket;
            auto groupIter = groupDetails.find(groupId);
            Group &group = groupIter->second;

            auto fileIter = group.files.find(fileName);

            if (fileIter != group.files.end())
            {
                FileDetails &fileDetails = fileIter->second;

                // Find the peerSocket in the peersSocket vector and remove it
                auto it = find(fileDetails.peersSocket.begin(), fileDetails.peersSocket.end(), peerSocket);
                if (it != fileDetails.peersSocket.end())
                {
                    fileDetails.peersSocket.erase(it);
                    string message = "PeerSocket entry removed from FileDetails. ❌!!";
                    createLogEntry(message);
                    message += "\n";
                    strcpy(buffer, message.c_str());
                    send(clientPort, buffer, sizeof(buffer), 0);
                    cout << buffer << endl;
                    continue;
                }
                else
                {

                    string message = "PeerSocket not found in the peersSocket vector. ❌!!";
                    createLogEntry(message);
                    message += "\n";
                    strcpy(buffer, message.c_str());
                    send(clientPort, buffer, sizeof(buffer), 0);
                    cout << buffer << endl;
                    continue;
                }
            }
            else
            {

                string message = "File " + fileName + " not found in the group. ❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
        }
        if (tokens[0] == "join_group")
        {
            int groupId = stoi(tokens[1]);
            if (!isUserLoggedIn(currUserId))
            {
                string message = "User is not Logged in ❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
            if (groupDetails.find(groupId) == groupDetails.end())
            {
                string message = "Group not exists ❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
            if (isUserInGroup(groupId, currUserId))
            {
                string message = "User already exists in the group 👍!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
            auto it = groupDetails.find(groupId);
            if (it != groupDetails.end())
            {
                it->second.joinRequests.insert(currUserId);
                string message = "User added to join Requests for the group ✅!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
            }
            // cout << "^^" << endl;
            // printGroupDetails(groupDetails);
        }
        if (tokens[0] == "leave_group")
        {
            int groupId = stoi(tokens[1]);
            if (!isUserLoggedIn(currUserId))
            {
                string message = "User is not Logged in ❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
            if (groupDetails.find(groupId) == groupDetails.end())
            {
                string message = "Group not exists ❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
            if (!isUserInGroup(groupId, currUserId))
            {
                string message = "You are not part of Group: " + to_string(groupId) + " ❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }

            Group &group = groupDetails[groupId];

            // Remove currUserId from members list

            // If currUserId is the admin, assign a new admin
            if (group.adminUserId == currUserId)
            {
                if (!group.members.empty())
                {
                    group.adminUserId = *group.members.begin();
                }
                else
                {
                    // If there are no members left, assign adminUserId to an empty string or some default value
                    groupDetails.erase(groupId);
                    string message = "Group: " + to_string(groupId) + " is removed ✅!!";
                    createLogEntry(message);
                    message += "\n";
                    strcpy(buffer, message.c_str());
                    send(clientPort, buffer, sizeof(buffer), 0);
                    cout << buffer << endl;

                    continue;
                }
            }
            else
            {
                group.members.erase(currUserId);
            }

            // Remove peerSockets entry with the provided socket in all the files
            for (auto &file : group.files)
            {
                FileDetails &fileDetails = file.second;
                auto it = find(fileDetails.peersSocket.begin(), fileDetails.peersSocket.end(), users[currUserId].socket);
                if (it != fileDetails.peersSocket.end())
                {
                    fileDetails.peersSocket.erase(it);
                }
            }
            string message = "You have left Group: " + to_string(groupId) + " ✅!!";
            createLogEntry(message);
            message += "\n";
            strcpy(buffer, message.c_str());
            send(clientPort, buffer, sizeof(buffer), 0);
            cout << buffer << endl;

            // printGroupDetails(groupDetails);
        }
        if (tokens[0] == "list_requests")
        {
            int groupId = stoi(tokens[1]);
            if (!isUserLoggedIn(currUserId))
            {
                string message = "User is not Logged in ❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
            if (groupDetails.find(groupId) == groupDetails.end())
            {
                string message = "Group not exists ❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
            Group &group = groupDetails[groupId];

            if (!isUserInGroup(groupId, currUserId))
            {
                string message = "You are not part of Group- " + to_string(groupId) + " ❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
            if (group.adminUserId != currUserId)
            {
                string message = "Only Admins are allowed to access pending joining requests❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }

            string message = "";

            for (const auto &request : group.joinRequests)
            {
                message += request + " "; // Concatenate join requests with space separator
            }

            // Remove the trailing space, if any
            if (!message.empty())
            {
                message.pop_back();
            }
            createLogEntry("Pending request list send succesfully✅!!");
            message = "Pending request list send succesfully:" + message;
            message += "\n";
            strcpy(buffer, message.c_str());
            send(clientPort, buffer, sizeof(buffer), 0);
            cout << buffer << endl;
        }
        if (tokens[0] == "accept_request")
        {
            int groupId = stoi(tokens[1]);
            string userId = tokens[2];

            if (!isUserLoggedIn(currUserId))
            {
                string message = "User is not Logged in ❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
            if (groupDetails.find(groupId) == groupDetails.end())
            {
                string message = "Group not exists ❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
            Group &group = groupDetails[groupId];

            if (group.adminUserId != currUserId)
            {
                string message = "Only Admins are allowed to access pending joining requests❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }

            if (!isUserInGroup(groupId, currUserId))
            {
                string message = "You are not part of Group- " + to_string(groupId) + " ❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
            auto joinRequestIt = group.joinRequests.find(userId);
            if (joinRequestIt == group.joinRequests.end())
            {
                string message = "User with user id " + userId + " is not present❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }

            // User found in joinRequests, move user to members list
            group.members.insert(userId);
            group.joinRequests.erase(joinRequestIt);

            string message = "User with ID " + userId + " has been added to the group members list.✅";
            createLogEntry(message);
            message += "\n";
            strcpy(buffer, message.c_str());
            send(clientPort, buffer, sizeof(buffer), 0);
            cout << buffer << endl;
            // printGroupDetails(groupDetails);
        }
        if (tokens[0] == "list_groups")
        {
            if (!isUserLoggedIn(currUserId))
            {
                string message = "User is not Logged in ❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
            string message = "";
            for (const auto &pair : groupDetails)
            {
                message += to_string(pair.first) + " ";
            }
            if (!message.empty())
            {
                message.pop_back();
            }
            createLogEntry("Group list send succesfully✅!!");
            message = "Group list send succesfully:" + message;
            message += "\n";
            strcpy(buffer, message.c_str());
            send(clientPort, buffer, sizeof(buffer), 0);
            cout << buffer << endl;
        }
        if (tokens[0] == "list_files")
        {
            int groupId = stoi(tokens[1]);

            if (!isUserLoggedIn(currUserId))
            {
                string message = "User is not Logged in ❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
            if (groupDetails.find(groupId) == groupDetails.end())
            {
                string message = "Group not exists ❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
            Group &group = groupDetails[groupId];

            if (!isUserInGroup(groupId, currUserId))
            {
                string message = "You are not part of Group- " + to_string(groupId) + " ❌!!";
                createLogEntry(message);
                message += "\n";
                strcpy(buffer, message.c_str());
                send(clientPort, buffer, sizeof(buffer), 0);
                cout << buffer << endl;
                continue;
            }
            const auto &filesMap = group.files;
            string message = "";
            for (const auto &filePair : filesMap)
            {
                message += filePair.first + " ";
            }

            if (!message.empty())
            {
                message.pop_back();
            }
            createLogEntry("File list send succesfully✅!!");
            message = "File list send succesfully:" + message;
            message += "\n";
            strcpy(buffer, message.c_str());
            send(clientPort, buffer, sizeof(buffer), 0);
            cout << buffer << endl;
        }
    }
}
void startTracker(int port)
{
    createLogEntry("staring tracker thread");
    // addStaticUsers();

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

    cout << "Tracker listening on port " << port << "...🚀" << endl;

    while (running)
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

        thread clientThread(handleClientRequest, clientSocket, clientAddress);
        clientThread.detach(); // Detach the thread to run independently
    }

    // Close the server socket (this part of the code is unreachable in this example)
    close(serverSocket);
    cout << "reaching";
}
