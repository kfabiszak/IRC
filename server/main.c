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
const int USER_SIZE = 50;
const int ROOM_SIZE = 50;
const int MESSAGE_SIZE = 100;
const int USERS_IN_ROOM = 25;
const int STRING_SIZE = 256;

struct Message {
    char*  text;
    UInt32 length;
    int    toSend;
};

struct User {
    char  ip[STRING_SIZE];
    char  login[STRING_SIZE];
    int   id;
    int   descriptor;
    int   isValid;
};

struct Room {
    int   id;
    int   size;
    int   full;
    int   isDirty;
    struct Message messages[MESSAGE_SIZE];
    struct User users[USERS_IN_ROOM];
};

struct Room rooms[ROOM_SIZE];

void initSockAddr() {
    sockAddr.sin_family = PF_INET;
    sockAddr.sin_port = htons(3011);
    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
}

void initStruct() {
    for(int r = 0; r < ROOM_SIZE; r++) {
        rooms[r].size = USERS_IN_ROOM;
        for(int u = 0; u < rooms[r].size; u++) {
            rooms[r].users[u].isValid = 0;
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

void saveMsg(int roomNumber, struct Message Message) {
    for(int m = 0; m < MESSAGE_SIZE; m++) {
        if(rooms[roomNumber].messages[m].toSend != 1) {
            printf("Saved message i:%d text:%s\n", m, Message.text);
            rooms[roomNumber].messages[m] = Message;
            rooms[roomNumber].messages[m].text = Message.text;
            rooms[roomNumber].messages[m].toSend = 2;
            rooms[roomNumber].isDirty = 1;
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    
    static struct timeval tTimeout;
    fd_set fsMask, fsRmask, fsWmask;
    
    initSockAddr();
    initStruct();
    int tmpSizeOfFileDescriptor;
    
    // socket
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
    
    // accept
    printf("#SERVER#\n");
    
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
            user = rooms[0].users[i];
            userDesc = user.descriptor;
            if(user.isValid) {
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
            
            struct User User;
            User.descriptor = clientSocketFileDescriptor;
            User.isValid = 1;
            strcpy( User.ip, inet_ntoa((struct in_addr)clientSockAddr.sin_addr));
            strcpy( User.login, "login");
            
            //Search empty slot for new user
            for(int i = 0; i < USERS_IN_ROOM; i++) {
                if(!rooms[0].users[i].isValid) {
                    rooms[0].users[i] = User;
                    printf("New User at %d with desc:%d\n", i, User.descriptor);
                    break;
                }
            }
        }
        
        for (int u = 0; u < USERS_IN_ROOM; u++) {
            user = rooms[0].users[u];
            userDesc = user.descriptor;
            if(user.isValid) {
                if (FD_ISSET(userDesc, &fsRmask)) {
                    char* readBuffer;
                    readMsg(userDesc, &readBuffer);
                    char* readHeader = "User_";
                    char* readResult = (char *) malloc(1 + strlen(readHeader)+ strlen(readBuffer) );
                    sprintf(readResult,"%s%d: %s", readHeader, userDesc, readBuffer);
                    printf("client_%d: %s\n", userDesc, readResult);
                    
                    struct Message message;
                    message.text = readResult;
                    message.length = strlen(readResult);
                    saveMsg(0, message);
                }
            }
        }
        
        for (int r = 0; r < ROOM_SIZE; r++) {
            room = rooms[r];
            if(room.isDirty == 1) {
                for(int u = 0; u < room.size; u++) {
                    if(room.users[u].isValid == 1) {
                        userDesc = room.users[u].descriptor;
                        FD_SET(userDesc, &fsMask);
                    }
                }
            }
            rooms[r].isDirty = 0;
        }
        
        for (int r = 0; r < ROOM_SIZE; r++) {
            room = rooms[r];
            for (int u = 0; u < room.size; u++) {
                user = room.users[u];
                userDesc = user.descriptor;
                if (user.isValid && FD_ISSET(userDesc, &fsWmask)) {
                    for(int m = 0; m < MESSAGE_SIZE; m++) {
                        message = room.messages[m];
                        if(message.toSend == 1) {
                            printf("sending5\n");
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
