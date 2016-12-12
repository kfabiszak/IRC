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
        return 1;
    }
    return 0;
}

void sendMsg (int socket, char* message, UInt32 length) {
    UInt32 sendLength = length;
    sendHeader(socket, sendLength);
    write(socket, message, sendLength);
}

int main(int argc, char *argv[]) {
    char* hostIp = "0.0.0.0";
    char* hostPort = "3014";
    struct hostent* host;
    host = gethostbyname(hostIp);
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in saddr;
    saddr.sin_family = PF_INET;
    saddr.sin_port = htons(atoi(hostPort));

    memcpy(&saddr.sin_addr.s_addr, host->h_addr, host->h_length);
    connect(fd, (struct sockaddr*) &saddr, sizeof(saddr));

    if(fork() == 0) {
        //write
        char* sendBuffer = "HelloServer";
        sendMsg(fd, sendBuffer, 11);

        //read
        char* readBuffer;
        readMsg(fd, &readBuffer);
        printf("client: %s\n",readBuffer);

        //write
        sendBuffer = "Hello";
        sendMsg(fd, sendBuffer, 5);
    }
    return 0;
}
