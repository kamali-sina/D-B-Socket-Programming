#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "game.h"

#define STDIN_FILENO 0

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

int find_char_in_string(char* str, char c){
    for (int i = 0 ; i < strlen(str) ; i++){
        if (str[i] == c)
            return i;
    }
    return -1;
}

char* strip_message(char* message){
    printf("full message is %s", message);
    for (int i = 0; i < strlen(message); i++){
        if (message[i] == '>'){
            char *buf = (char*) malloc(sizeof(char) * 60);
            int start_index = i + 2;
            int port_len = strlen(message) - start_index;
            memcpy(buf, &message[start_index], port_len);
            buf[port_len] = '\0';
            printf("message was %s \n", buf);
            return buf;
        }
    }
}

int accept_new_port(connection* server, int *id){
    for (int i = 0; i < strlen(server->message_buffer); i++){
        char subbuff[60];
        memcpy( subbuff, &server->message_buffer[i], strlen(GAME_CREATED));
        subbuff[strlen(GAME_CREATED)] = '\0';
        if (strcmp(subbuff, GAME_CREATED) == 0){
            int comma_loc = find_char_in_string(server->message_buffer, ',');
            int port_index = i + strlen(GAME_CREATED);
            int port_len = comma_loc - port_index;
            memcpy(subbuff, &server->message_buffer[port_index], port_len);
            subbuff[port_len] = '\0';
            int game_port = atoi(subbuff);
            int id_len = 1;
            memcpy(subbuff, &server->message_buffer[comma_loc+1], id_len);
            subbuff[id_len] = '\0';
            *id = atoi(subbuff);
            return game_port;
        }
    }
    return -1;
}

int is_input_valid(connection *server, int *request_sent){
    int gamenumber = atoi(server->message_buffer);
    if (gamenumber < 2 || gamenumber > 4){
        printf("input was not valid, gamemode can be 2, 3 or 4\n");
        server->sending_something_to = 0;
        return -1;
    }
    *request_sent = 1;
    return gamenumber;
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
    int game_port;
    int game_id;
    int game_mode = -1;
    int request_sent = 0;
    while (1) {
        if (request_sent == 0)
            printf("Please, type what game you want to join: \n");
        else
            printf("Waiting for players to fill queue...\n");
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
            game_mode = is_input_valid(&server, &request_sent);
        }

        if (FD_ISSET(server.socket, &read_fds)) {
            if (get_message_connection(&server, NO_LOGGER) == -1)
                DieWithError("server closed connection");
            game_port = accept_new_port(&server, &game_id);
            if (game_port == -1)
                DieWithError("Server Did not accept our request");
            break;
        }

        if (FD_ISSET(server.socket, &write_fds) && server.sending_something_to == 1) {
            if (send_to_connection(&server, node_name, NO_LOGGER) != 0){
                printf("could not send message to server, terminating\n");
                exit(0);
            }
        }
    }

    printf("starting %d player game on port %d and as player %d\n", game_mode,game_port, game_id);
    connection game_server;
    get_game_server(game_port, &game_server);
    connect_to_game_server(&game_server, game_id);
    if (game_id != 0){
        sendto(game_server.socket, "hello", 100, 0, (struct sockaddr*)&game_server.address, sizeof(game_server.address));
    }
    setup_game(game_mode);
    int max_sock_fd = game_server.socket;
    printf("--------------WELCOME----------------\n");
    printf("HOWTOPLAY: you give 3 inputs, y x (vert/horiz), with horiz is 0 and vert is 1\n");
    printf("HOWTOWIN: get squars, get points... player with highest points wins...\n");
    printf("YOU ARE PLAYER %d\n",game_id);
    printf("-------------------------------------\n");
    print_map(game_id);
    alarm(ALARM_TIME);
    while (1){
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(game_server.socket, &read_fds);
        FD_ZERO(&write_fds);
        FD_SET(game_server.socket, &write_fds);
        int activity = select( max_sock_fd+ 1, &read_fds, &write_fds, NULL, NULL);

        if (FD_ISSET(STDIN_FILENO, &read_fds)){
            if (handle_read_from_stdin(&game_server) != 0){
                printf("could not read from stdin\n");
                exit(1);
            }
            if (strcmp(game_server.message_buffer, "hello") == 0){
                continue;
            }
            if (turn != game_id){
                printf("it is not your turn!\n");
                game_server.sending_something_to = 0;
            }else
            if (is_game_input_valid(&game_server) != 0){
                game_server.sending_something_to = 0;
                printf("input is not valid\n");
            }else{
                if (register_move(game_server.message_buffer, &game_server, game_id) == 0){
                    print_map(game_id);
                }else{
                    printf("line is already drawn... try another line\n");
                    game_server.sending_something_to = 0;
                }
            }
        }

        if (FD_ISSET(game_server.socket, &read_fds)) {
            if (get_message_udp(&game_server) == -1)
                DieWithError("one player closed connection\n");
            if (strcmp(game_server.message_buffer, "hello") == 0){
                continue;
            }
            register_move(game_server.message_buffer, &game_server, game_id);
            if (game_id == 0){
                game_server.sending_something_to = 0;
            }
            print_map(game_id);
        }

        if (FD_ISSET(game_server.socket, &write_fds) && game_server.sending_something_to == 1) {
            if (send_message_udp(&game_server, node_name) != 0){
                printf("could not send message to reciever, terminating\n");
                exit(0);
            }
        }
    }
}