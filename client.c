#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define INADDR "127.0.0.1"

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

    int listner_id = socket(AF_INET, SOCK_STREAM,0);
    if (connect(listner_id, (struct sockaddr *) &addressPort, sizeof(addressPort)) > -1)
        printf("yay");
}