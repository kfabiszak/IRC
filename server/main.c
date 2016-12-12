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

void initSockAddr() {
    sockAddr.sin_family = PF_INET;
    sockAddr.sin_port = htons(3014);
    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
}

void childend(int signo) {
    wait(NULL);
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

void readHeader(int socket, unsigned int x, void* length) {
    int result;
    result = read(socket, length,  x);
    if (result < 1 )
    {
        printf("ERROR #2");
    }
}

int sendHeader(int socket, unsigned int x) {
    UInt32 conv = htonl(x);
    char *data = (char*)&conv;
    int size = sizeof(conv);
    int result;

    result = write(socket, data, size);
    if (result < 0) {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
            // use select() or epoll() to wait for the socket to be writable again
        }
        else if (errno != EINTR) {
            return -1;
        }
    }
    return 0;
}

int readMsg (int socket, char** result) {
        UInt32 length = 0;
        readHeader(socket, sizeof(length), (void*)(&length));
        length = ntohl(length);
        if(length > 0) {
            *result = malloc((length)*sizeof(char));
            readXBytes(socket, length, (void*)*result);
//            printf("length %lu\n", length);
//            printf("bufferEnd %s\n", *result);
            return 1;
        }
        return 0;
}

void sendMsg (int socket, char* message, UInt32 length) {
    UInt32 sendLength = length;
    sendHeader(socket, sendLength);
    write(socket, message, sendLength);
}

int main(int argc, char* argv[]) {

    signal(SIGCHLD, childend);
    initSockAddr();
    int tmpSizeOfFileDescriptor;

    // socket
    int clientSocketFileDescriptor;
    int serverSocketFileDescriptor = socket(PF_INET, SOCK_STREAM, 0);
    int on = 1;
    setsockopt(serverSocketFileDescriptor, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
    if (serverSocketFileDescriptor < 0) {
        perror("socket");
        exit(1);
    }

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

    // accept
    while(1) {
        tmpSizeOfFileDescriptor = sizeof(struct sockaddr);
        clientSocketFileDescriptor = accept(serverSocketFileDescriptor, (struct sockaddr*)&clientSockAddr, &tmpSizeOfFileDescriptor);
        if(fork() == 0){

            //read
            char* readBuffer;
            readMsg(clientSocketFileDescriptor, &readBuffer);
            printf("client: %s\n", readBuffer);

            //write
            char* sendBuffer = "HelloClient";
            sendMsg(clientSocketFileDescriptor, sendBuffer, 11);

            //read
            readMsg(clientSocketFileDescriptor, &readBuffer);
            printf("client: %s\n", readBuffer);

            close(serverSocketFileDescriptor);
            close(clientSocketFileDescriptor);
            exit(EXIT_SUCCESS);
        }
        close(clientSocketFileDescriptor);
    }
    close(serverSocketFileDescriptor);

    return 0;
}
