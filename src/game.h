#ifndef GAME_H
#define MESSAGE_H
#include "message_handler.h"
#include<signal.h>

#define SERVER_ADDR "127.0.0.1"
#define EMPTY_HOUSE 'E'
#define FULL_HOUSE 'F'
#define ALARM_TIME 25
#define MOVEMADE 69
#define MAX_PLAYERS 5

int turn = 1;
int game_dot_num = 0;
int game_num_of_players = 0;
char *table_vertical;
char *table_horizental;
char *table_houses;
//max number of players is 5
int scores[] = {0,0,0,0,0};

void turn_handler(int signum){
    turn = (turn + 1) % game_num_of_players;
    printf("turn is over, player %d's turn\n", turn);
    alarm(ALARM_TIME);
}

int get_game_server(int port, connection* game_server){
    struct sockaddr_in addressPort;
    addressPort.sin_family = AF_INET;
    addressPort.sin_port = htons(port);
    addressPort.sin_addr.s_addr = htonl(INADDR_ANY);
    game_server->socket = socket(AF_INET, SOCK_DGRAM,IPPROTO_UDP);
    int broadcastPermission = 1;
    if (setsockopt(game_server->socket, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission, sizeof(broadcastPermission)) < 0){
        DieWithError("setsockopt() failed");
    }
    game_server->address = addressPort;
    game_server->sending_something_to = 0;
    return 0;
}

int make_game_server(int port){
    connection serv;
    struct sockaddr_in addressPort;
    addressPort.sin_family = AF_INET;
    addressPort.sin_port = htons(port);
    addressPort.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.socket = socket(AF_INET, SOCK_DGRAM,0);
    int broadcastEnable = 1;
    serv.address = addressPort;
    serv.sending_something_to = 0;
    if (bind(serv.socket, (struct sockaddr*)&serv.address, sizeof(serv.address)) < 0){
        printf("game server connect failed\n");
        exit(-1);
    }
    printf("game server connected\n");
    return 0;
}

int connect_to_game_server(connection* game_server, int id){
    if (id == 0){
        if (bind(game_server->socket, (struct sockaddr*)&game_server->address, sizeof(game_server->address)) < 0){
            printf("game server connect failed\n");
            exit(-1);
        }
        printf("game server created\n");
        return 0;
    }
}

int setup_game(int num_of_players){
    signal(SIGALRM,turn_handler);
    game_dot_num = num_of_players + 1;
    game_num_of_players = num_of_players;
    int table_size = game_dot_num * (game_dot_num - 1);
    int house_size = (game_dot_num - 1) * (game_dot_num -1);
    table_horizental = (char *)malloc(sizeof(char) * table_size + 1);
    table_vertical = (char *)malloc(sizeof(char) * table_size + 1);
    table_houses = (char *)malloc(sizeof(char) * house_size + 1);
    table_houses[house_size] = '\0';
    table_horizental[table_size] = '\0';
    table_vertical[table_size] = '\0';
    for (int i = 0 ; i < table_size ; i++){
        if (i < house_size)
            table_houses[i] = EMPTY_HOUSE;
        table_vertical[i] = EMPTY_HOUSE;
        table_horizental[i] = EMPTY_HOUSE;
    }
    return 0;
}

int parse_message(char *message, int* i, int* j, int* horiz_vert){
    char *tmp = strdup(message);
    *i = atoi(strsep(&tmp, " "));
    *j = atoi(strsep(&tmp, " "));
    *horiz_vert = atoi(tmp);
}

int is_game_input_valid(connection* peer){
    int len = strlen(peer->message_buffer);
    if (len < 4){
        return -1;
    }
    int i,j,hv;
    parse_message(peer->message_buffer, &i, &j, &hv);
    if (hv < 0 || hv >1)
        return -1;
    if (hv == 0){
        if (i >= 0 && i < game_dot_num && j>=0 && j<game_dot_num -1)
            return 0;
        else
            return -1;
    }else{
        if (i >= 0 && i < game_dot_num - 1 && j>=0 && j<game_dot_num)
            return 0;
        else
            return -1;
    }
}

int is_horiz_house_below_made(int i, int j){
    if (table_vertical[i*(game_dot_num) + j] == FULL_HOUSE && 
        table_vertical[i*(game_dot_num) + j + 1] == FULL_HOUSE &&
        table_horizental[(i+1)*(game_dot_num - 1) + j] == FULL_HOUSE)
        return 0;
    else
        return -1;
}

int is_horiz_house_up_made(int i, int j){
    if (table_vertical[(i-1)*(game_dot_num) + j] == FULL_HOUSE && 
        table_vertical[(i-1)*(game_dot_num) + j + 1] == FULL_HOUSE &&
        table_horizental[(i-1)*(game_dot_num - 1) + j] == FULL_HOUSE)
        return 0;
    else
        return -1;
}

int is_vertical_house_left_made(int i, int j){
    if (table_vertical[(i)*(game_dot_num) + j - 1] == FULL_HOUSE && 
        table_horizental[(i)*(game_dot_num - 1) + j - 1] == FULL_HOUSE &&
        table_horizental[(i+1)*(game_dot_num - 1) + j - 1] == FULL_HOUSE)
        return 0;
    else
        return -1;
}

int is_vertical_house_right_made(int i, int j){
    if (table_vertical[(i)*(game_dot_num) + j + 1] == FULL_HOUSE && 
        table_horizental[(i)*(game_dot_num - 1) + j] == FULL_HOUSE &&
        table_horizental[(i+1)*(game_dot_num - 1) + j] == FULL_HOUSE)
        return 0;
    else
        return -1;
}

int is_house_made(int i, int j, int hv){
    int any = 0;
    if (hv == 0){
        if (i != game_dot_num - 1){
            if (is_horiz_house_below_made(i,j) == 0){
                table_houses[i*(game_dot_num - 1) + j] = turn + 48;
                any++;
            }
        }
        if (i != 0){
            if (is_horiz_house_up_made(i,j) == 0){
                table_houses[(i-1)*(game_dot_num - 1) + j] = turn + 48;
                any++;
            }
        }
    }else{
        if (j != game_dot_num - 1){
            if (is_vertical_house_right_made(i,j) == 0){
                table_houses[i*(game_dot_num - 1) + j] = turn + 48;
                any++;
            }
        }
        if (j != 0){
            if (is_vertical_house_left_made(i,j) == 0){
                table_houses[(i)*(game_dot_num - 1) + j - 1] = turn + 48;
                any++;
            }
        }
    }
    scores[turn] += any;
    if (any == 0){
        return -1;
    }else{
        return 0;
    }
}

int is_game_done(){
    int total_houses = (game_dot_num - 1) * (game_dot_num - 1);
    int max_index = 0;
    int sum = 0;
    int tie = 0;
    int counter = 0;
    for (int i = 0; i < total_houses ; i++){
        if (table_houses[i] != EMPTY_HOUSE)
            counter++;
    }
    for (int i = 0 ; i < MAX_PLAYERS ; i++){
        sum += scores[i];
        if (scores[i] > scores[max_index]){
            max_index = i;
        }
    }
    for (int i = 0 ; i < MAX_PLAYERS ; i++){
        if (scores[i] == scores[max_index] && i != max_index){
            tie = 1;
        }
    }
    if (sum >= total_houses || counter >= total_houses){
        if (tie == 1)
            return -2;
        return max_index;
    }else
        return -1;
}

void print_shit(int game_id){
    if (game_id == turn)
        printf("input your move: \n");
    else
        printf("Waiting for player %d's move... \n", turn);
}

void print_map(int id){
    for (int i = 0 ; i < 2 * game_dot_num ; i++){
        for (int j = 0 ; j < game_dot_num ; j++){
            if (i == 0){
                printf("   %c", j+48);
            }else{
                //line with O
                if (i%2 == 1){
                    if (j == 0){
                        printf("%d  O", i/2);
                    }else{
                        if (table_horizental[(i/2) * (game_dot_num - 1) + j - 1] == FULL_HOUSE){
                            printf("---O");
                        }else
                            printf("   O");
                    }
                }else{     //line with names and vert lines
                    char house_holder;
                    if (j==0){
                        house_holder = ' ';
                    }else{
                        if (table_houses[((i/2) - 1) * (game_dot_num- 1) + j - 1] != EMPTY_HOUSE)
                            house_holder = table_houses[((i/2) - 1) * (game_dot_num- 1) + j - 1];
                        else
                            house_holder = ' ';
                    }
                    if (table_vertical[((i/2) - 1) * game_dot_num + j] == FULL_HOUSE)
                        printf(" %c |",house_holder);
                    else
                        printf(" %c  ",house_holder);
                }
            }
        }
        printf("\n");
    }
    print_shit(id);
}

void end_game(int winner, char* message, connection* peer,int id){
    send_message_udp(peer,"fuck");
    print_map(id);
    if (winner == -2){
        printf("we have no clear winner! it's a tie! GG.\n");
    }else{
        printf("we have a winner! well played player %d! GG.\n", winner);
    }
    exit(0);
}

int register_move(char *message, connection* peer, int id){
    int i, j, hv;
    parse_message(message, &i, &j, &hv);
    if (hv == 0){
        if (table_horizental[i*(game_dot_num - 1) + j] == EMPTY_HOUSE)
            table_horizental[i*(game_dot_num - 1) + j] = FULL_HOUSE;
        else
            return -1;
    }else{
        if (table_vertical[i*(game_dot_num) + j] == EMPTY_HOUSE)
            table_vertical[i*(game_dot_num) + j] = FULL_HOUSE;
        else
            return -1;
    }
    if (is_house_made(i,j,hv) == 0){
        int winner = is_game_done();
        if (winner != -1){
            end_game(winner, message, peer, id);
        }
        printf("player %d has made a block... they get to mark again.\n", turn);
        alarm(ALARM_TIME);
    }else{
        printf("player %d has not made a block... passing turn to next player\n", turn);
        turn_handler(MOVEMADE);
    }
    return 0;
}



#endif