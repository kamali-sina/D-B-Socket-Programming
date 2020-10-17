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
#include "logger.h"

#define NO_LOGGER -1

typedef struct {
    int socket;
    struct sockaddr_in address;
    int sending_something_to;
    char message_buffer[100];
    int game_mode;
} connection;

int get_message_connection(connection *peer, int logger_fd){
    int count = recv(peer->socket, peer->message_buffer, 100, MSG_DONTWAIT);
    if (count == 0){
        logger_log("empty message\n", logger_fd);
        return -1;
    }
    char message[100];
    snprintf(message, sizeof(message),"%s\n",peer->message_buffer, peer->address.sin_port);
    if (NO_LOGGER == logger_fd){
        printf("%s", message);
        return 0;
    }
    logger_log(message, logger_fd);
    return 0;
}

int send_to_connection(connection* peer,char *node_name, int logger_fd){
    char message[100];
    snprintf(message, sizeof(message),"%s -> %s", node_name, peer->message_buffer);
    send(peer->socket, message, 100, 0);
    peer->sending_something_to = 0;
    char log[100];
    snprintf(log, sizeof(log),"sent message %s to sin_port %d\n", peer->message_buffer, peer->address.sin_port);
    if (NO_LOGGER == logger_fd){
        printf("%s", log);
        return 0;
    }
    logger_log(log, logger_fd);
    return 0;
}