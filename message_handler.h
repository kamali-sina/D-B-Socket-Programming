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
#define GAME_CREATED "game ready on port:"

typedef struct {
    int socket;
    struct sockaddr_in address;
    int sending_something_to;
    char message_buffer[100];
    int game_mode;
} connection;

int ports[] = {-1,-1,-1,-1};

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

int add_if_needed(int sin){
    for (int i=0; i<4;i++){
        if (ports[i] == sin){
            return -1;
        }
    }
    for (int i= 0 ; i < 4; i++){
        if (ports[i] == -1){
            ports[i] = sin;
            return 0;
        }
    }
}

int send_to_all_except(int sin, connection* peer){
    for (int i= 0 ; i < 4; i++){
        if (ports[i] != -1 && ports[i]!= sin){
            peer->address.sin_port = ports[i];
            sendto(peer->socket, peer->message_buffer, 100, 0, (struct sockaddr*)&peer->address, sizeof(peer->address));
        }
    }
}

int get_message_udp (connection* peer){
    int len = sizeof(peer->address);
    int count = recvfrom(peer->socket, peer->message_buffer, 100, 0, (struct sockaddr*)&peer->address, &len);
    int sin = peer->address.sin_port;
    if (add_if_needed(sin) == -1){
        send_to_all_except(sin, peer);
    }
    if (count == 0){
        printf("empty message\n");
        return -1;
    }
    printf("recieved message %s\n",peer->message_buffer);
    if (count == 0){
        printf("fuck!\n");
        return -1;
    }
    return 0;
}

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

int send_message_udp (connection* peer, char* node_name){
    int count = sendto(peer->socket, peer->message_buffer, 100, 0, (struct sockaddr*)&peer->address, sizeof(peer->address));
    int sin = peer->address.sin_port;
    send_to_all_except(sin, peer);
    printf("sent a message to %d\n", peer->address.sin_port);
    peer->sending_something_to = 0;
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