# P2P

## Distributed File System

- The Distributed File Sharing System is a network-based application that allows peers to share and retrieve files across multiple peers over a network. It utilizes a socket programming and client-server architecture and employs secure communication protocols for data transfer.

## How to Run

### to run client

```
1. cd client
2. make
3. ./client​ <IP>:<PORT> <TRACKER INFO FILE>
    ex: ./client 127.0.0.1:6000 tracker_info.txt
```

### to run Tracker

```
1. cd tracker
2. make
3. ./tracker​ <TRACKER INFO FILE> <TRACKER NUMBER>
   ex: ./tracker tracker_info.txt 1
```

`<TRACKER INFO FILE>` contains the IP, Port details of the tracker.

```
Ex:
127.0.0.1:5000
127.0.0.1:6000
```

1 Create user account:

```
create_user​ <user_id> <password>
```

2 Login:

```
login​ <user_id> <password>
```

3 Create Group:

```
create_group​ <group_id>
```

4 Join Group:

```
join_group​ <group_id>
```

5 Leave Group:

```
leave_group​ <group_id>
```

6 List pending requests:

```
list_requests ​<group_id>
```

7 Accept Group Joining Request:

```
accept_request​ <group_id> <user_id>
```

8 List All Group In Network:

```
list_groups
```

9 List All sharable Files In Group:

```
list_files​ <group_id>
```

10 Upload File:

```
​upload_file​ <file_path> <group_id​>
```

11 Download File:​

```
download_file​ <group_id> <file_name> <destination_path>
```

12 Logout:​

```
logout
```

13 Stop share file:​

```
stop_share <group_id> <file_name>
```

14 Show Downloads:​

```
 show_downloads
```

## Workflow

1. Users are required to create an account and register with the tracker system.

2. Once registered, users can log in using their credentials.

3. The tracker system maintains client information along with the files shared by clients. This information assists clients in peer-to-peer communication.

4. Users have the capability to create groups, becoming the administrators of these groups.

5. Users can retrieve a list of all existing groups stored on the server.

6. Users have the flexibility to join or leave groups as per their preference.

7. Group administrators have the authority to accept or reject group join requests.

8. Within a group, users can share files. The sharing process includes transmitting the filename and its corresponding SHA1 hash for file integrity verification.

9. Users can access a list of all shareable files within a specific group.

10. **Download Process:**

    - Retrieve peer information for the desired file from the tracker.
    - Implement a piece selection algorithm that chooses the rarest piece first and downloads it from a randomly selected peer possessing that piece.
    - Download different pieces of the file from multiple peers simultaneously. All downloaded files are shareable within the same group. File integrity is verified using SHA1 comparison.

11. **Piece Selection Algorithm:**

    - The algorithm selects the rarest piece and downloads it from a randomly chosen peer possessing that piece.

12. Upon logout, all shared files are no longer accessible, and the user's entry is removed from the group.

## Assumptions

1. File names are unique within each group.
2. File and group names cannot contain spaces.
3. All upload paths provided by users are valid.
4. Users cannot join a group if they are already a member.
5. Users can upload a file only if they possess the complete file.
6. Group administrators must join the group using the join command.
7. If an admin leaves the group, another random member will assume the role of the group's administrator. If no members remain, the group will be removed from the system.
8. Folder must we there to download in particular destination path.
