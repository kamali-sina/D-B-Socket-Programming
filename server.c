#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define INADDR "127.0.0.1"

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
    addressPort.sin_addr.s_addr = inet_addr(INADDR);

    int socket_id = socket(PF_INET, SOCK_STREAM,0);
    int x = bind(socket_id, (struct sockaddr *) &addressPort, sizeof(addressPort));
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(socket_id, &read_fds);
    int fdmax = socket_id;


// loop forever
    while( 1 )
    {
        if (select(fdmax+1, &read_fds, NULL,NULL,NULL) == -1){
            perror("select");
            exit(4);
        }

        for (int i = 0; i<= fdmax; i++){
            printf("Testing: %d, %d\n", i, FD_ISSET(i,&read_fds));

        }
    }
}