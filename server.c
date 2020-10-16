#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>


#define SERVER_IPV4_ADDR "127.0.0.1"
#define MAX_CLIENTS 20
#define NO_SOCKET -1

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

typedef struct {
    int socket;
    struct sockaddr_in address;
    size_t current_sending_byte;
    size_t current_receiving_byte;
} peer_t;

peer_t connection_list[MAX_CLIENTS];

int start_listen_socket(int *listen_sock, int listen_port){
    *listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (*listen_sock < 0) {
        perror("socket");
        return -1;
    }
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = inet_addr(SERVER_IPV4_ADDR);
    my_addr.sin_port = htons(listen_port);
    if (bind(*listen_sock, (struct sockaddr*)&my_addr, sizeof(struct sockaddr)) != 0) {
        perror("bind");
        return -1;
    }
    
    // start accept client connections
    if (listen(*listen_sock, 10) != 0) {
        perror("listen");
        return -1;
    }
    printf("Accepting connections on port %d.\n", (int)listen_port);
    return 0;
}

int handle_new_connection(int listen_sock)
{
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t client_len = sizeof(client_addr);
    int new_client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_len);
    if (new_client_sock < 0) {
        perror("accept()");
        return -1;
    }
    char client_ipv4_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ipv4_str, INET_ADDRSTRLEN);
    printf("Incoming connection from %s:%d.\n", client_ipv4_str, client_addr.sin_port);
    int i;
    for (i = 0; i < MAX_CLIENTS; ++i) {
        if (connection_list[i].socket == NO_SOCKET) {
            connection_list[i].socket = new_client_sock;
            connection_list[i].address = client_addr;
            connection_list[i].current_sending_byte   = -1;
            connection_list[i].current_receiving_byte = 0;
            return 0;
        }
    }
}

int main(int argc, char *argv[]){
    if (argc < 2){
        printf("No Command Line Arguments were provided\n");
        exit(0);
    }


    int listen_port = atoi(argv[1]);
    int listen_sock;
    if (start_listen_socket(&listen_sock, listen_port) == -1){
        exit(0);
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(listen_sock, &read_fds);
    printf("this is listen_sock: %d\n",listen_sock);
    int fdmax = listen_sock;

    while( 1 )
    {
        FD_ZERO(&read_fds);
        FD_SET(listen_sock, &read_fds);
        for (int i = 0; i < MAX_CLIENTS; i++)
            FD_SET(connection_list[i].socket, &read_fds);

        // for (int i = 0; i < fdmax+1; i++){
        //     printf("status of %d is %d\n",i,FD_ISSET(i,&read_fds));
        // }
        int status = select(fdmax+1, &read_fds, NULL,NULL,NULL);
        if (status == -1){
            printf("fuck\n");
            return -1;
        }else if (status == 0){
            printf("you shouldnt have done that\n");
            return 0;
        }
        if (FD_ISSET(listen_sock,&read_fds)){
            handle_new_connection(listen_sock);
        }
    }
}