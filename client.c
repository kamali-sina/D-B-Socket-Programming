#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "message_handler.h"

#define SERVER_ADDR "127.0.0.1"
#define STDIN_FILENO 0

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

int read_from_stdin(char *read_buffer, size_t max_len){
    memset(read_buffer, 0, max_len);
    
    ssize_t read_count = 0;
    ssize_t total_read = 0;
    read_count = read(STDIN_FILENO, read_buffer, max_len);
    size_t len = strlen(read_buffer);
    if (len > 0 && read_buffer[len - 1] == '\n')
        read_buffer[len - 1] = '\0';

    printf("Read from stdin %s .\n", read_buffer);
    return 0;
}

int handle_read_from_stdin(connection* peer){
    read_from_stdin (peer->message_buffer, 100);
    peer->sending_something_to = 1;
    return 0;
}

int make_server(connection* server, int port_no){
    struct sockaddr_in addressPort;
    addressPort.sin_family = AF_INET;
    addressPort.sin_port = htons(port_no);
    addressPort.sin_addr.s_addr = htonl(INADDR_ANY);
    server->socket = socket(AF_INET, SOCK_STREAM,0);
    server->address = addressPort;
    server->sending_something_to = 0;
    return 0;
}

int main(int argc, char *argv[]){
    if (argc < 2){
        printf("No Command Line Arguments were provided\n");
        exit(0);
    }
    if (argc == 2){
        printf("you have not provided a name, we will use name \"unnamed\" for you.\n");
    }
    int port_no = atoi(argv[1]);
    char node_name[100] = "unnamed";
    connection server;
    make_server(&server, port_no);
    
    if (argc == 3){
        snprintf(node_name,sizeof(node_name), argv[2]);
    }
    if (connect(server.socket, (struct sockaddr *) &server.address, sizeof(server.address)) != 0)
        DieWithError("connect");
    printf("%s Connected to %s:%d.\n", node_name,SERVER_ADDR, port_no);
    fd_set read_fds;
    fd_set write_fds;
    int maxfd = server.socket;
    
    while (1) {
        printf("Waiting for server message or stdin input. Please, type text to send:\n");
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(server.socket, &read_fds);

        FD_ZERO(&write_fds);
        if (server.sending_something_to == 1)
            FD_SET(server.socket, &write_fds);
            
        int activity = select(maxfd + 1, &read_fds, &write_fds, NULL, NULL);

        if (FD_ISSET(STDIN_FILENO, &read_fds)){
            if (handle_read_from_stdin(&server) != 0){
                printf("could not read from stdin\n");
                exit(1);
            }
        }

        if (FD_ISSET(server.socket, &read_fds)) {
            if (get_message_connection(&server, NO_LOGGER) == -1)
                DieWithError("server closed connection");
        }

        if (FD_ISSET(server.socket, &write_fds) && server.sending_something_to == 1) {
            if (send_to_connection(&server, node_name, NO_LOGGER) != 0){
                printf("could not send message to server, terminating\n");
                exit(0);
            }
        }
    }
}