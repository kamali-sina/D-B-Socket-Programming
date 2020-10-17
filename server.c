#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h> 
#include "message_handler.h"

#define SERVER_IPV4_ADDR "127.0.0.1"
#define MAX_CLIENTS 20
#define STDIN_FILENO 0
#define NO_SOCKET -1

char server_name[30] = "server";
int play_queue[3] = { 0, 0, 0 };
int last_port_used = 2020;

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

connection connection_list[MAX_CLIENTS];

void init_connections(){
    for (int i = 0; i < MAX_CLIENTS; i++){
        connection_list[i].socket = NO_SOCKET;
    }
}

int start_listen_socket(int *listen_sock, int listen_port, int logger_fd){
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
    // we use 10 because maximum number of people in queue will be 1+2+3 = 6
    if (listen(*listen_sock, 10) != 0) {
        perror("listen");
        return -1;
    }
    char message[100];
    snprintf(message, sizeof(message),"Accepting connections on port %d.\n", (int)listen_port);
    logger_log(message, logger_fd);
    return 0;
}

void close_client_connection(connection* peer, int logger_fd){
    char message[100];
    snprintf(message, sizeof(message),"closed connection on port %d\n", peer->address.sin_port);
    logger_log(message,logger_fd);
    peer->socket = NO_SOCKET;
}

int handle_new_connection(int listen_sock, int logger_fd){
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
    char message [100];
    snprintf(message, sizeof(message), "Incoming connection from %s:%d.\n", client_ipv4_str, client_addr.sin_port);
    logger_log(message, logger_fd);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (connection_list[i].socket == NO_SOCKET) {
            connection_list[i].socket = new_client_sock;
            connection_list[i].address = client_addr;
            connection_list[i].sending_something_to = 0;
            return 0;
        }
    }
    logger_log("could not save the new client.\n", logger_fd);
    return -1;
}

int parse_message(connection* peer){
    for (int i = 0; i < strlen(peer->message_buffer); i++){
        if (peer->message_buffer[i] == '>'){
            char c = peer->message_buffer[i+2];
            peer->game_mode = c - 48;
            play_queue[peer->game_mode - 2] += 1;
            if (play_queue[peer->game_mode - 2] == peer->game_mode){
                printf("ready to start %d player game...\n", peer->game_mode);
                return peer->game_mode;
            }
            return -1;
        }
    }
}

int reset_fds(fd_set *read_fds, fd_set *write_fds, int listen_sock){
    int fdmax = listen_sock;
    FD_ZERO(read_fds);
    FD_SET(STDIN_FILENO, read_fds);
    FD_SET(listen_sock, read_fds);
    for (int i = 0; i < MAX_CLIENTS; i++){
        if (connection_list[i].socket != NO_SOCKET){
            FD_SET(connection_list[i].socket, read_fds);
            if (connection_list[i].socket > fdmax)
                fdmax = connection_list[i].socket;
        }
    }
    FD_ZERO(write_fds);
    for (int i = 0; i < MAX_CLIENTS; ++i)
        if (connection_list[i].socket != NO_SOCKET && connection_list[i].sending_something_to == 1){
            FD_SET(connection_list[i].socket, write_fds);
        }

    return fdmax;
}

int get_free_port(){
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }
    
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = inet_addr(SERVER_IPV4_ADDR);
    int i;
    for (i = last_port_used+1; i < 10000; i++){
        my_addr.sin_port = htons(0);
        if (bind(sock, (struct sockaddr*)&my_addr, sizeof(struct sockaddr)) == 0) {
            break;
        }
    }
    last_port_used = i;
    return i;
}

int queue_messages(int gamemode, int port, int logger_fd){
    int ins = 0;
    for (int i = 0 ; i < MAX_CLIENTS ; i++){
        if (connection_list[i].game_mode == gamemode && connection_list[i].socket != NO_SOCKET){
            snprintf(connection_list[i].message_buffer, sizeof(connection_list[i].message_buffer), "%s%d,%d",GAME_CREATED,port,ins);
            connection_list[i].sending_something_to = 1;
            ins += 1;
        }
    }
    if (ins != gamemode){
        logger_log("encountered bug in queuing messages\n", logger_fd);
        exit(1);
    }
    logger_log("messages queued to start game\n",logger_fd);
    play_queue[gamemode - 2] = 0;
    return 0;
}

int main(int argc, char *argv[]){
    if (argc < 2){
        printf("No Command Line Arguments were provided\n");
        exit(0);
    }
    init_connections();
    int logger_fd = logger_make("server_log.txt");
    int listen_port = atoi(argv[1]);
    int listen_sock;
    if (start_listen_socket(&listen_sock, listen_port, logger_fd) == -1){
        logger_log("could not listen on given port\n", logger_fd);
        exit(0);
    }

    fd_set read_fds;
    fd_set write_fds;
    int fdmax = listen_sock;
    
    while(1){
        fdmax = reset_fds(&read_fds, &write_fds, listen_sock);
        int status = select(fdmax+1, &read_fds, &write_fds,NULL,NULL);
        if (status == -1){
            logger_log("fuck, select status was -1\n",logger_fd);
            return -1;
        }else if (status == 0){
            logger_log("you shouldnt have done that, select status was 0\n", logger_fd);
            return 0;
        }
        if (FD_ISSET(listen_sock,&read_fds)){
            if (handle_new_connection(listen_sock, logger_fd) != 0){
                logger_log("Could not handle new connection, Terminating\n",logger_fd);
                exit(0);
            }
        }
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (connection_list[i].socket != NO_SOCKET && FD_ISSET(connection_list[i].socket, &read_fds)) {
                if (get_message_connection(&connection_list[i], logger_fd) != 0) {
                    close_client_connection(&connection_list[i], logger_fd);
                    continue;
                }
                int gamemode = parse_message(&connection_list[i]); 
                if (gamemode != -1){
                    int port = get_free_port();
                    queue_messages(gamemode, port, logger_fd);
                }
            }
    
            if (connection_list[i].socket != NO_SOCKET && FD_ISSET(connection_list[i].socket, &write_fds)) {
                if (send_to_connection(&connection_list[i], server_name, logger_fd) != 0) {
                    close_client_connection(&connection_list[i], logger_fd);
                    continue;
                }
                close_client_connection(&connection_list[i], logger_fd);
            }
        }

    }
}