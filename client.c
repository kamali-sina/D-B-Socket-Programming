#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define SERVER_ADDR "127.0.0.1"
#define STDIN_FILENO 0

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

int main(int argc, char *argv[]){
    if (argc < 2){
        printf("No Command Line Arguments were provided\n");
        exit(0);
    }

    int port_no = atoi(argv[1]);

    struct sockaddr_in addressPort;
    addressPort.sin_family = AF_INET;
    addressPort.sin_port = htons(port_no);
    addressPort.sin_addr.s_addr = htonl(INADDR_ANY);

    int server_sock = socket(AF_INET, SOCK_STREAM,0);
    if (connect(server_sock, (struct sockaddr *) &addressPort, sizeof(addressPort)) != 0)
        DieWithError("connect");
    printf("Connected to %s:%d.\n", SERVER_ADDR, port_no);
    fd_set read_fds;
    int maxfd = server_sock;
    // printf("Waiting for server message or stdin input. Please, type text to send:\n");
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(server_sock, &read_fds);
            
        int activity = select(maxfd + 1, &read_fds, NULL, NULL, NULL);

        if (FD_ISSET(server_sock, &read_fds)) {
            char buf[100];
            int count = recv(server_sock, &buf, 100, MSG_DONTWAIT);
            if (count == 0)
                DieWithError("recieve");
            printf("got message\"%s\"\n",buf);
        }
    }
}