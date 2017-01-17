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
#include <signal.h>

static volatile int keepRunning = 1;

typedef unsigned int UInt32;

struct sockaddr_in sockAddr, clientSockAddr;
#define QUEUE_SIZE 10      // Kolejka multipleksera
#define USER_SIZE 40       // Maksymalna liczba urzytkowników będących jednocześnie na serwerze
#define ROOM_SIZE 20       // Maksymalna ilość pokoi
#define MESSAGE_SIZE 500   // Maksymalna wielkość wysyłanej wiadomości
#define USERS_IN_ROOM 10   // Maksymalna liczba urzytkowników w pokoju
#define LOGIN_SIZE 15      // Maksymalna długość loginu urzytkownika

// Struktura wiadomości
// Zmienne cmd, arg, text służą do przechowywania rozbitej wiadomości.
// Zmienna length oznacza aktualną długość wiadomości.
// Flaga toSend oznacza wiadomość gotową do rozesłania.
struct Message {
    char*  name;
    char*  cmd;
    char*  arg;
    char*  text;
    UInt32 length;
    int    toSend;
    int    desc;
};

// Struktura usera
// Zmienna login zawiera nazwę urzytkownika.
// Flaga isValid nadawana jest po poprawnym dołączeniu na serwer.
struct User {
    char  ip[32];
    char  login[LOGIN_SIZE];
    int   descriptor;
    int   isValid;
};

// Głowną strukturą są pokoje zawierające pamięc zaalokowaną dla wiadomości, oraz tablicę wskaźników na usera.
// Dołączenie usera do pokoju następuje poprzez ustawienie wskaźnika na usera w danym pokoju.
// Flaga pokoju isDirty oznacza czy pokój posiada jakieś wiadomości gotowe do rozesłania.
// Zmienna size oznacza maksymalną ilość urzytkowników w danym pokoju.
struct Room {
    int   size;
    int   isDirty;
    struct Message messages[MESSAGE_SIZE];
    struct User **users;
};

struct Room rooms[ROOM_SIZE];   // Tablica zawierająca wystąpienia pokoi


void initSockAddr();
void my_handler(int signal);
void initStruct();
void readHeader(int socket, unsigned int x, void* length);
void readXBytes(int socket, unsigned int x, void* buffer);
int readMsg (int socket, char** result);
int sendHeader(int socket, unsigned int x);
int sendMsg (int socket, char* message, UInt32 length);
void decodeMSG(struct Message *Message);
void saveMsg(int roomNumber, struct Message *message);
int addUserToRoom(struct User *user, int roomNumber);
void sendResponse(char* msg, int userDesc);
void sendResponseJoin(int state, int userDesc, int roomNumber);
void sendResponseLeave(int state, int userDesc, int roomNumber);
void sendResponseLogin(int state, struct User *user);
void sendResponseConnect(int state, int userDesc);
int clearUser(int userDesc, int roomNumber);
int removeUserFromRoom(struct User *user, int roomNumber);
int logoutUser(struct User *user);
int changeUserLogin(struct User *user, char* newLogin);
int checkLoginUnique(char* login);
char* getUserList();
char* getUserRoomList(int roomNumber);
void sendUserList(int desc, int roomNumber);
void sendUserListToAllRooms(int desc);
struct User *findUser(int descriptor);
int checkUserInRoom(int descriptor, int roomNumber);
void runCmd(int userDesc, char *msg);

void initSockAddr() {
    sockAddr.sin_family = PF_INET;
    sockAddr.sin_port = htons(3012);
    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
}

void my_handler(int signal){
    printf("Server closing... by signal: %d\n",signal);
    keepRunning = 0;
}

// Inicjalizacja podstawowych struktur serwera
// Alokowanie pamięci
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

// Odbieranie wiadomości o wielkości kolejnej wiadomości
void readHeader(int socket, unsigned int x, void* length) {
    int result;
    result = (int)read(socket, length, x);
    if (result < 1 )
    {
        printf("ERROR #2 Read header");
    }
}

// Odczytanie x kolejnych bajtów
void readXBytes(int socket, unsigned int x, void* buffer) {
    int bytesRead = 0;
    int result;
    while (bytesRead < x)
    {
        result = (int)read(socket, buffer + bytesRead, x - bytesRead);
        if (result < 1 )
        {
            printf("ERROR #1 Read x bytes");
            break;
        }
        bytesRead += result;
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
    if (x > 0) { // Połączenie jest poprawne.
    } else if (x == 0) { // Wylogowanie przez urzytkownika
        logoutUser(findUser(socket));
        return 0;
    } else { // Nieoczekiwane rozłączenie z urzytkownikiem
        logoutUser(findUser(socket));
        return 0;
    }
    UInt32 length = 0;
    readHeader(socket, sizeof(length), (void*)(&length));
//    printf("$$User %d: size=%d\n", socket, length);
    length = ntohl(length); // comment with java client !
    if(length > 0 && length <= 256) {
        *result = malloc((length)*sizeof(char));
        readXBytes(socket, length, (void*)*result);
        return 1;
    }
    return 0;
}

// Przesłanie wiadomości z wielkością kolejnej wiadomości
int sendHeader(int socket, unsigned int x) {
    UInt32 conv = htonl(x);
    char *data = (char*)&conv;
    int size = sizeof(conv);
    int result;
    printf(">>User %d: Sent message: size=%d bytes=%d\n", socket, x, size);
    result = (int)write(socket, data, size);
    if (result < 0) {
        printf("Error #3 Send header\n");
        return 0;
    }
    return 1;
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
        else if (p == 1) {
            Message->arg = header;
        } else if (p == 2) {
            Message->text += (3 + strlen(Message->cmd) + strlen(Message->arg)); //todo message length > 0
            break;
        }
        header = strtok (NULL, "#");
    }
}

// Zapisanie wiadomości do późniejszego wysłania
// * odnalezienie wolnej struktury na wiadomość
// * zapisanie wiadomości
// * ustawienie numeru pokoju docelowego
// * ustawienie flagi isDirty
void saveMsg(int roomNumber, struct Message *message) {
    for(int m = 0; m < MESSAGE_SIZE; m++) {
        if(rooms[roomNumber].messages[m].toSend != 1) {
            memset(rooms[roomNumber].messages[m].text,0,strlen(rooms[roomNumber].messages[m].text));
            rooms[roomNumber].messages[m].name = message->name;
            rooms[roomNumber].messages[m].length = message->length;
            
            if(strlen(message->name) > 0) { //Message for client
                char* result = (char *) malloc(8 + strlen(message->name)+ strlen(message->text) );
                sprintf(result, "#send#%d#%s#%s", roomNumber, message->name, message->text);
                strcpy(rooms[roomNumber].messages[m].text, result);
                rooms[roomNumber].messages[m].length = (UInt32)strlen(result);
            } else { //Response from server
                strcpy(rooms[roomNumber].messages[m].text, message->text);
            }
            
            rooms[roomNumber].messages[m].toSend = 2;
            rooms[roomNumber].isDirty = 1;
            printf("||Room %d: Saved message: '%s'\n", roomNumber, rooms[roomNumber].messages[m].text);
            break;
        }
    }
}

// Dodanie usera do pokoju:
// * walidacja usera: czy został poprawnie przyjęty na serwer oraz czy nie jest już w danym pokoju
// * walidacja pokoju
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
                printf("##User %d: join to room %d.\n", rooms[roomNumber].users[u]->descriptor, roomNumber);
                sendUserList(rooms[roomNumber].users[u]->descriptor, roomNumber);
                return 1;
                
            }
            else if(u < USERS_IN_ROOM && rooms[roomNumber].users[u]->isValid == 0) {
                rooms[roomNumber].users[u] = user;
                printf("##User %d: join to room %d.\n", user->descriptor, roomNumber);
                sendUserList(rooms[roomNumber].users[u]->descriptor, roomNumber);
                return 1;
            }
        }
    printf("##User %d: cannot join to room %d.\n", user->descriptor, roomNumber);
    return 0;
}

// Wysłanie odpowiedzi serwera do urzytkownika o poprawności wykonania akcji:
// * state
//  * 1) success
//  * 2) error
// wiadomości dotyczą statusu dołączenia/opuszczenia pokoju, zmiany loginu, połączenia z serwerem
void sendResponse(char* msg, int userDesc) {
    printf(">>User %d: Sent response: '%s'\n", userDesc, msg);
    sendMsg(userDesc, msg, (UInt32)strlen(msg));
}
void sendResponseJoin(int state, int userDesc, int roomNumber) {
    char* head;
    if(state)
        head = "#success#join#";
    else
        head = "#error#join#";
    char* result = malloc((strlen(head)+2) * sizeof(char)); //todo size of room number
    sprintf(result, "%s%d", head, roomNumber);
    sendResponse(result, userDesc);
}
void sendResponseLeave(int state, int userDesc, int roomNumber) {
    char* head;
    if(state)
        head = "#success#leave#";
    else
        head = "#error#leave#";
    char* result = malloc((strlen(head)+2) * sizeof(char));
    sprintf(result, "%s%d", head, roomNumber);
    sendResponse(result, userDesc);
}
void sendResponseLogin(int state, struct User *user) {
    char* head;
    if(state)
        head = "#success#login#";
    else
        head = "#error#login#";
    char* result = malloc(((strlen(head))+strlen(user->login)) * sizeof(char));
    sprintf(result, "%s%s", head, user->login);
    sendResponse(result, user->descriptor);
}
void sendResponseConnect(int state, int userDesc) {
    char* result;
    if(state)
        result = "#success#connect#";
    else
        result = "#error#connect#";
    sendResponse(result, userDesc);
}

// Wyczyszczenie informacji usera z servera
// zwolnienie miejsc w pokojach zajmowanych przez usera
int clearUser(int userDesc, int roomNumber) {
    for(int u = 0; u < rooms[roomNumber].size; u++) {
        if(rooms[roomNumber].users[u]->isValid == 1 && (rooms[roomNumber].users[u]->descriptor == userDesc)) {
            rooms[roomNumber].users[u] = malloc(sizeof(struct User));
            rooms[roomNumber].users[u]->isValid = 0;
            rooms[roomNumber].users[u]->descriptor = -1;
            return 1;
        }
    }
    return 0;
}

// Usunięcie usera z pokoju
int removeUserFromRoom(struct User *user, int roomNumber) {
    if(roomNumber > 0) {
        printf("##User %d leave from room %d.\n", user->descriptor, roomNumber);
        return clearUser(user->descriptor, roomNumber);
    } else
        return 0;
}

// Wylogowanie usera
int logoutUser(struct User *user) {
    for(int r = ROOM_SIZE-1; r >= 0; r--) {
        if(clearUser(user->descriptor, r)) {
            sendUserList(user->descriptor, r);
        }
    }
    close(user->descriptor);
    printf("##User %d logout from server.\n", user->descriptor);
    return 1;
}

// Zmiana loginu usera
int changeUserLogin(struct User *user, char* newLogin) {
    unsigned long int loginSize = strlen(newLogin);
    if(loginSize > 0 && loginSize < LOGIN_SIZE && checkLoginUnique(newLogin)) {
        printf("##User %d change login '%s' to '%s'\n", user->descriptor, user->login, newLogin);
        strcpy(user->login, newLogin);
        sendUserListToAllRooms(user->descriptor);
        return 1;
    } else
        printf("##User %d cannot change login '%s' to '%s'\n", user->descriptor, user->login, newLogin);
    return 0;
}

// Sprawdzenie czy login jest zajęty
int checkLoginUnique(char* login) {
    for(int u = 0; u < rooms[0].size; u++) {
        if(strncmp(login, rooms[0].users[u]->login, LOGIN_SIZE) == 0) {
            return 0;
        }
    }
    return 1;
}

// Pobranie globalnej listy urzytknowników: #users#0#nick1#nick2...
char* getUserList() {
    char* usersList = malloc(((USER_SIZE+1)*LOGIN_SIZE) * sizeof(char)); //todo room count size to string
    strncpy(&usersList[0], "#users#0#", 9);
    for(int u = 0; u < rooms[0].size; u++) {
        if(rooms[0].users[u]->isValid) {
            
            char* name = rooms[0].users[u]->login;
            char name2[LOGIN_SIZE+1];
            printf("%s-", name);
            sprintf(name2, "%s#", name);
            strncpy(&usersList[strlen(usersList)], name2, strlen(name2));
        }
    }
    strncpy(&usersList[strlen(usersList)], " #", 2);
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
    strncpy(&usersList[strlen(usersList)], " #", 2);
    return usersList;
}

// Wysłanie wiadomości z listą urzytkowników do danego pokoju
void sendUserList(int desc, int roomNumber) {
    struct Message message;
    message.name = "";
    message.desc = desc;
    message.text = getUserRoomList(roomNumber);
    message.length = (UInt32)strlen(message.text);
    saveMsg(roomNumber, &message);
}

// Wysłanie wiadomości z listą urzytkowników do pokojów z danym urzytkownikiem
void sendUserListToAllRooms(int desc) {
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
    return NULL;
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
    message.cmd = "";
    message.arg = "";
    message.text = "";
    
    message.text = msg;
    message.length = (UInt32)strlen(msg);
    message.desc = userDesc;
    message.name = findUser(userDesc)->login;
    
    decodeMSG(&message);
    
    int roomNumber = atoi(message.arg);
    
    if(strcmp(message.cmd, "send") == 0) {
        if(checkUserInRoom(userDesc, roomNumber))
            saveMsg(roomNumber, &message);
    } else if(strcmp(message.cmd, "login") == 0) {
        struct User *user = findUser(userDesc);
        int state = changeUserLogin(user, message.arg);
        sendResponseLogin(state, user);
    } else if(strcmp(message.cmd, "logout") == 0) {
        logoutUser(findUser(userDesc));
    } else if(strcmp(message.cmd, "join") == 0) {
        int state = addUserToRoom(findUser(userDesc), roomNumber);
        sendResponseJoin(state, userDesc, roomNumber);
    } else if(strcmp(message.cmd, "leave") == 0) {
        int state = removeUserFromRoom(findUser(userDesc), roomNumber);
        sendResponseLeave(state, userDesc, roomNumber);
        if (state)
            sendUserList(userDesc, roomNumber);
    }
}

int main(int argc, char* argv[]) {
    
    static struct timeval tTimeout;
    fd_set fsMask, fsRmask, fsWmask;
    
    initSockAddr();
    initStruct();
    
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    
    unsigned int tmpSizeOfFileDescriptor;
    
    int clientSocketFileDescriptor;
    int serverSocketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    int on = 1;
    int activity;
    
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
    
    tmpSizeOfFileDescriptor = sizeof(struct sockaddr);
    
    printf("#SERVER RUNNING#\n");
    printf("To close server press ctrl + c\n");
    //main loop
    while(keepRunning) {
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
        
        if(keepRunning) {
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
                strcpy( User.login, "system");
                
                int state = addUserToRoom(&User, 0);
                sendResponseConnect(state, User.descriptor);
                if(!state) {
                    close(User.descriptor);
                }
            }
        }
        
        //READ MSG FROM ALL VALID USERS
        for (int u = 0; u < USER_SIZE; u++) {
            user = *rooms[0].users[u];
            userDesc = user.descriptor;
            if(user.isValid == 1) {
                if (FD_ISSET(userDesc, &fsRmask)) {
                    char* readBuffer;
                    if(readMsg(userDesc, &readBuffer)) {
                        printf("<<User %d: Received: '%s'\n", userDesc, readBuffer);
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
        
        //IF USER HAS SET WRITE MASK, SEND MESSAGE TO THAT USER
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
    
    FD_ZERO(&fsRmask);
    FD_ZERO(&fsWmask);
    FD_ZERO(&fsMask);
    for(int u = 0; u < USER_SIZE; u++) {
        user = *rooms[0].users[u];
        if(user.isValid)
            close(user.descriptor);
    }
    close(serverSocketFileDescriptor);
    return 0;
}
