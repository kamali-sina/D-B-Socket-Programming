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

int logger_make(char *filename){
    int fd = creat(filename, S_IRUSR | S_IWUSR);
    if (fd >= 0)
        return fd;
    fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd < 0)
        printf("cannot open logger file\n");
    return fd;
}

int logger_log(char *message, int fd){
    printf("%s", message);
    return write(fd, message, strlen(message));
}