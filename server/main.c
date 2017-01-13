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
const int QUEUE_SIZE = 10;      // Kolejka multipleksera
const int USER_SIZE = 20;       // Maksymalna liczba urzytkowników będących jednocześnie na serwerze
const int ROOM_SIZE = 40;       // Maksymalna ilość pokoi
const int MESSAGE_SIZE = 500;   // Maksymalna wielkość wysyłanej wiadomości
const int USERS_IN_ROOM = 10;   // Maksymalna liczba urzytkowników w pokoju
const int LOGIN_SIZE = 15;      // Maksymalna długość loginu urzytkownika

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
    char  ip[32];
    char  login[LOGIN_SIZE];
    int   descriptor;
    int   isValid;
};

struct Room {
    int   id;
    int   size;
    int   full;
    int   isDirty;
    struct Message messages[MESSAGE_SIZE];
    struct User **users;
};

struct Room rooms[ROOM_SIZE];

int clearUser(int userDesc, int roomNumber);
char* getUserList();
char* getUserRoomList(int roomNumber);
void sendUserList(int desc, int room);
void saveMsg(int roomNumber, struct Message *message);
int checkUserInRoom(int descriptor, int roomNumber);
int logoutUser(struct User *user);
struct User *findUser(int descriptor);
void sendUserListToAllRooms(int desc);

void initSockAddr() {
    sockAddr.sin_family = PF_INET;
    sockAddr.sin_port = htons(3012);
    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
}

// Inicjalizacja podstawowych struktur serwera
void initStruct() {
    int size = USER_SIZE;
    for(int r = 0; r < ROOM_SIZE; r++) {
        rooms[r].users = malloc(sizeof(struct User) * size);
        rooms[r].isDirty = 0;
        rooms[r].size = size;
        for(int u = 0; u < size; u++) {
            rooms[r].users[u] = malloc(sizeof(struct User));
            rooms[r].users[u]->isValid = 0;
            rooms[r].users[u]->descriptor = -1;
        }
        for(int m = 0; m < MESSAGE_SIZE; m++) {
            rooms[r].messages[m].text = malloc((MESSAGE_SIZE + 1) * sizeof(char));
            rooms[r].messages[m].toSend = 0;
        }
        if(r == 0)
            size = USERS_IN_ROOM;
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
//        printf("BR: %d\n", bytesRead);
    }
}

// Odbieranie wiadomości:
// * sprawdzenie stanu połączenia z klientem
//     * rozłączenie usera przy braku połączenia
// * odebranie headera
// * odebranie wiadomości o długości przesłanej w headerze
int readMsg (int socket, char** result) {
    
    char c;
    ssize_t x = recv(socket, &c, 1, MSG_PEEK);
    if (x > 0) {
        printf("Podtrzymanie z %d\n", socket);
    } else if (x == 0) {
        printf("Rozłączenie z %d\n", socket);
        logoutUser(findUser(socket));
        close(socket);
        return 0;
    } else {
        printf("Error z %d\n", socket);
        return 0;
    }
    
    UInt32 length = 0;
    readHeader(socket, sizeof(length), (void*)(&length));
    length = ntohl(length); // comment with java client !
    printf("HEADER2: %u\n", length);
    if(length > 0 && length <= 256) {
        printf("HEADER!: %u\n", length);
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
    printf("SENDING TO %d size:%d bytes:%d\n", socket, x, size);
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

// Wysyłanie wiadomości poprzez:
// * wysłanie headera
// * wysłanie tekstu wiadomości
int sendMsg (int socket, char* message, UInt32 length) {
    UInt32 sendLength = length;
    sendHeader(socket, sendLength);
    if(write(socket, message, sendLength) < 0) {
        printf("Problem with send #2\n");
        return 0;
    }
    return 1;
}

// Dekodowanie wiadomości poprzez podział przez znak '#'
// Ustawienie komendy i argumentów wiadomości
void decodeMSG(struct Message *Message) {
    char* header;
    char* dividedMsg = (char *) malloc(strlen(Message->text));
    strcpy(dividedMsg, Message->text);
    header = strtok (dividedMsg, "#");
    
    for (int p = 0; header != NULL; p++) {
        if (p == 0)
            Message->cmd = header;
        else if (p == 1)
            Message->arg = header;
        else if (p == 2) {
            Message->text += (3 + strlen(Message->cmd) + strlen(Message->arg));
            break;
        }
        header = strtok (NULL, "#");
    }
}

// Zapisanie wiadomości do późniejszego wysłania
void saveMsg(int roomNumber, struct Message *message) {
   
    for(int m = 0; m < MESSAGE_SIZE; m++) {
        if(rooms[roomNumber].messages[m].toSend != 1) {
            rooms[roomNumber].messages[m].name = message->name;
            rooms[roomNumber].messages[m].length = message->length;
            if(strlen(message->name) > 0) {
                //Message from client
                char* result = (char *) malloc(10 + strlen(message->name)+ strlen(message->text) );
                sprintf(result, "#send#%d#%s#%s\0", roomNumber, message->name, message->text);
                strcpy(rooms[roomNumber].messages[m].text, result);
                rooms[roomNumber].messages[m].length += 10;
            } else {
                //Message from server
                strcpy(rooms[roomNumber].messages[m].text, message->text);
            }
            rooms[roomNumber].messages[m].toSend = 2;
            rooms[roomNumber].isDirty = 1;
            printf("#Saved message i:%d text:'%s'.\n", m, rooms[roomNumber].messages[m].text);
            break;
        }
    }
}

// Dodanie usera do pokoju:
// * pokoj = 0 -> Inicjalizacja usera dodanie do pokoju głownego
// * pokoj > 0 -> Dodanie usera do pokoju podrzędnego
// * wysłanie wiadomości do wszystkich w danym pokoju z listą userów
int addUserToRoom(struct User *user, int roomNumber) {
    if(roomNumber < ROOM_SIZE && user->isValid && !checkUserInRoom(user->descriptor, roomNumber))
        for(int u = 0; u < rooms[roomNumber].size; u++) {
            if(roomNumber == 0 && rooms[roomNumber].users[u]->isValid == 0) {
                rooms[roomNumber].users[u]->descriptor = user->descriptor;
                strcpy(rooms[roomNumber].users[u]->ip, user->ip);
                strcpy(rooms[roomNumber].users[u]->login, user->login);
                rooms[roomNumber].users[u]->isValid = 1;
                printf("#New User %d join to room %d.\n", rooms[roomNumber].users[u]->descriptor, roomNumber);
                sendUserList(rooms[roomNumber].users[u]->descriptor, roomNumber);
                return 1;
                
            }
            else if(u < USERS_IN_ROOM && rooms[roomNumber].users[u]->isValid == 0) {
                rooms[roomNumber].users[u] = user;
                printf("#User %d join to room %d.\n", user->descriptor, roomNumber);
                sendUserList(rooms[roomNumber].users[u]->descriptor, roomNumber);
                return 1;
            }
        }
    printf("#User %d cannot join to room %d.\n", user->descriptor, roomNumber);
    return 0;
}

// Wysłanie wiadomości potwierdzającej wykonanie akcji;
void sendSuccess(int userDesc) {
    sendMsg(userDesc, "#succes#", 8);
}

// Usunięcie informacji usera z servera
int clearUser(int userDesc, int roomNumber) {
    for(int u = 0; u < rooms[roomNumber].size; u++) {
        if(rooms[roomNumber].users[u]->isValid == 1 && (rooms[roomNumber].users[u]->descriptor == userDesc)) {
            rooms[roomNumber].users[u] = malloc(sizeof(struct User));
            rooms[roomNumber].users[u]->isValid = 0;
            rooms[roomNumber].users[u]->descriptor = -1;
            printf("CZYSZCZENIE %d", roomNumber);
            return 1;
        }
    }
    return 0;
}

// Usunięcie usera z pokoju
int removeUserFromRoom(struct User *user, int roomNumber) { // TODO usuwa całego usera zamiast tylko z pokoju
    printf("#User %d leave from room %d.\n", user->descriptor, roomNumber);
    return clearUser(user->descriptor, roomNumber);
}

// Wylogowanie usera
int logoutUser(struct User *user) {
    for(int r = ROOM_SIZE-1; r >= 0; r--) {
        if(clearUser(user->descriptor, r)) {
            sendUserList(user->descriptor, r);
        }
    }
    printf("#User %d logout from server.\n", user->descriptor);
    return 1;
}

// Zmiana loginu usera
int changeUserLogin(struct User *user, char* newLogin) {
    unsigned long int loginSize = strlen(newLogin);
    if(loginSize > 0 && loginSize < LOGIN_SIZE) {
        printf("#User %d change login '%s' to '%s'.\n", user->descriptor, user->login, newLogin);
        strcpy(user->login, newLogin);
        sendUserListToAllRooms(user->descriptor);
        return 1;
    } else
        printf("#User %d cannot change login '%s' to '%s'.\n", user->descriptor, user->login, newLogin);
    return 0;
}

// Pobranie globalnej listy urzytknowników: #users#nick1#nick2...
char* getUserList() {
    char* usersList = malloc(((USER_SIZE+1)*LOGIN_SIZE) * sizeof(char));
    strncpy(&usersList[0], "#users#0#", 9);
    for(int u = 0; u < rooms[0].size; u++) {
        if(rooms[0].users[u]->isValid) {
            char* name = rooms[0].users[u]->login;
            char name2[LOGIN_SIZE+1];
            sprintf(name2, "%s#", name);
            strncpy(&usersList[strlen(usersList)], name2, strlen(name2));
        }
    }
    strncpy(&usersList[strlen(usersList)], "\0", 1);
    return usersList;
}

// Pobranie listy urzytkowników będących w danym pokoju: #users#room_number#nick1#nick2...
char* getUserRoomList(int roomNumber) {
    if (roomNumber == 0)
        return getUserList();
    char* usersList = malloc(((USER_SIZE+1)*LOGIN_SIZE) * sizeof(char));
    strncpy(&usersList[0], "#users#", 7);
    char roomNumberToString[5];
    sprintf(roomNumberToString, "%d#", roomNumber);
    strncpy(&usersList[strlen(usersList)], roomNumberToString, strlen(roomNumberToString));
    for(int u = 0; u < rooms[roomNumber].size; u++) {
        if(rooms[roomNumber].users[u]->isValid) {
            char* name = rooms[roomNumber].users[u]->login;
            char name2[LOGIN_SIZE+1];
            sprintf(name2, "%s%s", name, "#");
            strncpy(&usersList[strlen(usersList)], name2, strlen(name2));
        }
    }
    strncpy(&usersList[strlen(usersList)], "\0", 1);
    return usersList;
}

// Wysłanie wiadomości z listą urzytkowników do danego pokoju
void sendUserList(int desc, int roomNumber){
    struct Message message;
    message.name = "";
    message.desc = desc;
    message.text = getUserRoomList(roomNumber);
    message.length = strlen(message.text);
    saveMsg(roomNumber, &message);
}

// Wysłanie wiadomości z listą urzytkowników do pokojów z danym urzytkownikiem
void sendUserListToAllRooms(int desc){
    for(int r = 0; r < ROOM_SIZE; r++) {
        if(checkUserInRoom(desc, r))
            sendUserList(desc, r);
    }
}

// Znalezienie urzytkownika po deskryptorze
struct User *findUser(int descriptor) {
    for (int u = 0 ; u < USER_SIZE ; u++) {
        if(rooms[0].users[u]->descriptor == descriptor) {
            return rooms[0].users[u];
        }
    }
    struct User empty;
    return &empty;
}

// Sprawdzenie czy użytkownik o danym deskryptorze znajduje się w pokoju
int checkUserInRoom(int descriptor, int roomNumber) {
    for(int u = 0 ; u < rooms[roomNumber].size ; u++) {
        if(rooms[roomNumber].users[u]->descriptor == descriptor) {
            return 1;
        }
    }
    return 0;
}

// Dekodowanie wiadomości
// Wykonanie odpowiedniego zadania przekazanego przez wiadomość
void runCmd(int userDesc, char *msg) {
    struct Message message;
    message.text = msg;
    message.length = strlen(msg);
    message.desc = userDesc;
    message.name = findUser(userDesc)->login;
    
    decodeMSG(&message);
    
    int roomNumber = atoi(message.arg);
    
    if(strcmp(message.cmd, "send") == 0) {
        if(checkUserInRoom(userDesc, roomNumber)) // todo ?
            saveMsg(roomNumber, &message);
    } else if(strcmp(message.cmd, "login") == 0) {
        if(changeUserLogin(findUser(userDesc), message.arg))
            sendSuccess(message.desc);
    } else if(strcmp(message.cmd, "logout") == 0) {
        if(logoutUser(findUser(userDesc))) {
            sendSuccess(message.desc);
            close(message.desc);
        }
    } else if(strcmp(message.cmd, "join") == 0) {
        if(addUserToRoom(findUser(userDesc), roomNumber))
            sendSuccess(message.desc);
        
    } else if(strcmp(message.cmd, "leave") == 0) {
        if(roomNumber > 0)
            if(removeUserFromRoom(findUser(userDesc), roomNumber)) {
                sendSuccess(message.desc);
                sendUserList(message.desc, roomNumber);
            }
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
                fprintf(stderr, "%s: Cannot create a connection's socket.\n", argv[0]);
                exit(1);
            }
            
            //CREATE NEW USER
            struct User User;
            User.descriptor = clientSocketFileDescriptor;
            User.isValid = 1;
            strcpy( User.ip, inet_ntoa((struct in_addr)clientSockAddr.sin_addr));
            strcpy( User.login, "noname");
            
            addUserToRoom(&User, 0);
        }
        
        //READ MSG FROM ALL VALID USERS
        for (int u = 0; u < USER_SIZE; u++) {
            user = *rooms[0].users[u];
            userDesc = user.descriptor;
            if(user.isValid == 1) {
                if (FD_ISSET(userDesc, &fsRmask)) {
                    char* readBuffer;
                    if(readMsg(userDesc, &readBuffer)) {
                        printf("client_%d: %s\n", userDesc, readBuffer);
                        runCmd(userDesc, readBuffer);
                    }
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
