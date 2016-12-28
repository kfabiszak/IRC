#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

typedef char Int8;
typedef int Int32;

typedef unsigned char UInt8;
typedef unsigned int UInt32;

struct sockaddr_in sockAddr, clientSockAddr;
const int QUEUE_SIZE = 10;
const int USER_SIZE = 25;
const int ROOM_SIZE = 50;
const int MESSAGE_SIZE = 100;
const int USERS_IN_ROOM = 25;
const int STRING_SIZE = 256;

struct Message {
    char*  name;
    char*  cmd;
    char*  arg;
    char*  text;
    UInt32 length;
    int    toSend;
    int    desc;
};

struct User {
    char  ip[STRING_SIZE];
    char  login[STRING_SIZE];
    int   descriptor;
    int   isValid;
};

struct Room {
    int   id;
    int   size;
    int   full;
    int   isDirty;
    struct Message messages[MESSAGE_SIZE];
    struct User *users[USERS_IN_ROOM];
};

struct Room rooms[ROOM_SIZE];

void initSockAddr() {
    sockAddr.sin_family = PF_INET;
    sockAddr.sin_port = htons(3011);
    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
}

void initStruct() {
    for(int r = 0; r < ROOM_SIZE; r++) {
        rooms[r].isDirty = 0;
        rooms[r].size = USERS_IN_ROOM;
        for(int u = 0; u < rooms[r].size; u++) {
            rooms[r].users[u] = malloc(sizeof(struct User));
            rooms[r].users[u]->isValid = 0;
            rooms[r].users[u]->descriptor = -1;
        }
        for(int m = 0; m < MESSAGE_SIZE; m++) {
            rooms[r].messages[m].toSend = 0;
        }
    }
}

void readHeader(int socket, unsigned int x, void* length) {
    int result;
    result = read(socket, length, x);
    if (result < 1 )
    {
        printf("ERROR #2");
    }
}

void readXBytes(int socket, unsigned int x, void* buffer) {
    int bytesRead = 0;
    int result;
    while (bytesRead < x)
    {
        result = read(socket, buffer + bytesRead, x - bytesRead);
        if (result < 1 )
        {
            printf("ERROR #1");
            break;
        }
        bytesRead += result;
    }
}

int readMsg (int socket, char** result) {
    UInt32 length = 0;
    readHeader(socket, sizeof(length), (void*)(&length));
    length = ntohl(length);
    if(length > 0) {
        *result = malloc((length)*sizeof(char));
        readXBytes(socket, length, (void*)*result);
        return 1;
    }
    return 0;
}

int sendHeader(int socket, unsigned int x) {
    UInt32 conv = htonl(x);
    char *data = (char*)&conv;
    int size = sizeof(conv);
    int result;
    printf("SENDING TO %d text:%d size:%d\n", socket, x, size);
    result = write(socket, data, size);
    if (result < 0) {
        printf("Problem with send #1\n");
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
            // use select() or epoll() to wait for the socket to be writable again
        }
        else if (errno != EINTR) {
            return -1;
        }
    }
    return 0;
}

int sendMsg (int socket, char* message, UInt32 length) {
    UInt32 sendLength = length;
    sendHeader(socket, sendLength);
    if(write(socket, message, sendLength) < 0) {
        printf("Problem with send #2\n");
        return 0;
    }
    return 1;
}

void decodeMSG(struct Message *Message) {
    char* header;
    header = strtok (Message->text,"#");
    for (int p = 0; header != NULL; p++) {
        switch(p) {
            case 0:
                Message->cmd = header;
                break;
            case 1:
                Message->arg = header;
                break;
            default: //todo cutting text on #
                Message->text = header;
                break;
        }
        header = strtok (NULL, "#");
    }
}

void addUserToRoom(struct User *user, int roomNumber) { //#todo validate roomNumber
    for(int u = 0; u < USERS_IN_ROOM; u++) {
        if(roomNumber == 0 && rooms[roomNumber].users[u]->isValid == 0) {
            rooms[roomNumber].users[u]->descriptor = user->descriptor;
            strcpy(rooms[roomNumber].users[u]->ip, user->ip);
            strcpy(rooms[roomNumber].users[u]->login, user->login);
            rooms[roomNumber].users[u]->isValid = 1;
            printf("#New User %d join to room %d.\n", rooms[roomNumber].users[u]->descriptor, roomNumber);
            break;
            
        }
        else if(rooms[roomNumber].users[u]->isValid == 0) {
            rooms[roomNumber].users[u] = user;
            printf("#User %d join to room %d.\n", user->descriptor, roomNumber);
            break;
        }
    }
}

void clearUser(int userDesc, int roomNumber) {
    for(int u = 0; u < USERS_IN_ROOM; u++) {
        if(rooms[roomNumber].users[u]->isValid == 1 && (rooms[roomNumber].users[u]->descriptor == userDesc)) {
            rooms[roomNumber].users[u]->isValid = 0;
            strcpy(rooms[roomNumber].users[u]->ip, "");
            strcpy(rooms[roomNumber].users[u]->login, "");
            rooms[roomNumber].users[u]->descriptor = NULL;
            break;
        }
    }
}

void removeUserFromRoom(struct User *user, int roomNumber) {
    clearUser(user->descriptor, roomNumber);
    printf("#User %d leave from room %d.\n", user->descriptor, roomNumber);
}

void changeUserLogin(struct User *user, char* newLogin) {
    printf("#User %d change login '%s' to '%s'.\n", user->descriptor, user->login, newLogin);
    strcpy(user->login, newLogin);
}

struct User *findUser(int descriptor) {
    for (int u = 0 ; u < USER_SIZE ; u++) {
        if(rooms[0].users[u]->descriptor == descriptor) {
            return rooms[0].users[u];
        }
    }
    struct User empty;
    return &empty;
}

int checkUserInRoom(int descriptor, int roomNumber) {
    for(int u = 0 ; u < USERS_IN_ROOM ; u++) {
        if(rooms[roomNumber].users[u]->descriptor == descriptor) {
            return 1;
        }
    }
    return 0;
}

void saveMsg(int roomNumber, struct Message Message) {
    for(int m = 0; m < MESSAGE_SIZE; m++) {
        if(rooms[roomNumber].messages[m].toSend != 1) {
            rooms[roomNumber].messages[m] = Message;
            rooms[roomNumber].messages[m].name = Message.name;
            char* result = (char *) malloc(1 + strlen(Message.name)+ strlen(Message.text) );
            sprintf(result, "%s: %s", Message.name, Message.text);
            strcpy(rooms[roomNumber].messages[m].text, result);
            rooms[roomNumber].messages[m].toSend = 2;
            rooms[roomNumber].isDirty = 1;
            printf("#Saved message i:%d text:'%s'.\n", m, rooms[roomNumber].messages[m].text);
            break;
        }
    }
}

void runCmd(int userDesc, char *msg) {
    struct Message message;
    message.text = msg;
    message.length = strlen(msg);
    message.desc = userDesc;
    message.name = findUser(userDesc)->login;
    
    decodeMSG(&message);

    int roomNumber = atoi(message.arg);
    
    if(strcmp(message.cmd, "send") == 0) {
        if(checkUserInRoom(userDesc, roomNumber))
            saveMsg(roomNumber, message);
    } else if(strcmp(message.cmd, "login") == 0) {
        changeUserLogin(findUser(userDesc), message.arg);
    } else if(strcmp(message.cmd, "logout") == 0) {
        
    } else if(strcmp(message.cmd, "join") == 0) {
        addUserToRoom(findUser(userDesc), roomNumber);
    } else if(strcmp(message.cmd, "leave") == 0) {
        removeUserFromRoom(findUser(userDesc), roomNumber);
    }
}

int main(int argc, char* argv[]) {
    
    static struct timeval tTimeout;
    fd_set fsMask, fsRmask, fsWmask;
    
    initSockAddr();
    initStruct();
    int tmpSizeOfFileDescriptor;
    
    int clientSocketFileDescriptor;
    int serverSocketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    int on = 1;
    int activity, nMaxfd;
    
    struct Room room;
    struct Message message;
    struct User user;
    int userDesc;
    int maxDesc;
    
    if (serverSocketFileDescriptor < 0) {
        perror("socket");
        exit(1);
    }
    
    setsockopt(serverSocketFileDescriptor, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
    
    // bind
    if (bind(serverSocketFileDescriptor, (struct sockaddr*)&sockAddr, sizeof(struct sockaddr)) < 0) {
        perror("bind");
        close(serverSocketFileDescriptor);
        exit(1);
    }
    
    // listen
    if (listen(serverSocketFileDescriptor, QUEUE_SIZE) < 0) {
        perror("listen");
        close(serverSocketFileDescriptor);
        exit(1);
    }
    
    FD_ZERO(&fsMask);
    FD_ZERO(&fsRmask);
    nMaxfd = serverSocketFileDescriptor;
    tmpSizeOfFileDescriptor = sizeof(struct sockaddr);
    
    printf("#SERVER RUNNING#\n");
    
    //main loop
    while(1) {
        //copy the write set
        fsWmask = fsMask;
        
        //clear the socket set
        FD_ZERO(&fsRmask);
        
        //add server socket to set
        FD_SET(serverSocketFileDescriptor, &fsRmask);
        maxDesc = serverSocketFileDescriptor;
        
        //add client sockets to set
        for (int i = 0 ; i < USER_SIZE ; i++) {
            user = *rooms[0].users[i];
            userDesc = user.descriptor;
            if(user.isValid == 1) {
                if(userDesc > 0)
                    FD_SET(userDesc , &fsRmask);
                if(userDesc > maxDesc)
                    maxDesc = userDesc;
            }
        }
        
        tTimeout.tv_sec = 5;
        tTimeout.tv_usec = 0;
        activity = select(maxDesc + 1, &fsRmask, &fsWmask, (fd_set*) 0, &tTimeout);
        
        if (activity < 0) {
            fprintf(stderr, "%s: Select error.\n", argv[0]);
        }
        if (activity == 0) {
            printf("Timed out.\n");
            fflush(stdout);
        }
        
        if (FD_ISSET(serverSocketFileDescriptor, &fsRmask)) {
            clientSocketFileDescriptor = accept(serverSocketFileDescriptor, (struct sockaddr*)&clientSockAddr, &tmpSizeOfFileDescriptor);
            if (clientSocketFileDescriptor < 0) {
                fprintf(stderr, "%s: Can't create a connection's socket.\n", argv[0]);
                exit(1);
            }
            
            //CREATE NEW USER
            struct User User;
            User.descriptor = clientSocketFileDescriptor;
            User.isValid = 1;
            strcpy( User.ip, inet_ntoa((struct in_addr)clientSockAddr.sin_addr));
            strcpy( User.login, "");
            
            addUserToRoom(&User, 0);
        }
        
        //READ MSG FROM ALL VALID USERS
        for (int u = 0; u < USERS_IN_ROOM; u++) {
            user = *rooms[0].users[u];
            userDesc = user.descriptor;
            if(user.isValid == 1) {
                if (FD_ISSET(userDesc, &fsRmask)) {
                    char* readBuffer;
                    readMsg(userDesc, &readBuffer);
                    printf("client_%d: %s\n", userDesc, readBuffer);
                    
                    runCmd(userDesc, readBuffer);
                }
            }
        }
        
        //IF ROOM IS DIRTY SET EACH USER MASK TO WRITE
        for (int r = 0; r < ROOM_SIZE; r++) {
            room = rooms[r];
            if(room.isDirty) {
                for(int u = 0; u < room.size; u++) {
                    if(room.users[u]->isValid == 1) {
                        userDesc = room.users[u]->descriptor;
                        FD_SET(userDesc, &fsMask);
                    }
                }
            }
            rooms[r].isDirty = 0;
        }
        
        //IF USER HAS SET WRITE MASK SEND MSG TO THAT USER
        for (int r = 0; r < ROOM_SIZE; r++) {
            room = rooms[r];
            for (int u = 0; u < room.size; u++) {
                user = *room.users[u];
                userDesc = user.descriptor;
                if(user.isValid == 1)
                    if (FD_ISSET(userDesc, &fsWmask)) {
                        for(int m = 0; m < MESSAGE_SIZE; m++) {
                            message = room.messages[m];
                            if(message.toSend) {
                                sendMsg(userDesc, message.text, message.length);
                            }
                        }
                        FD_CLR(userDesc, &fsMask);
                    }
            }
            for (int m = 0; m < room.size; m++) {
                if(room.messages[m].toSend > 0)
                    rooms[r].messages[m].toSend -= 1;
            }
        }
    }
    
    close(serverSocketFileDescriptor);
    
    return 0;
}
