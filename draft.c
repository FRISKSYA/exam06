#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct s_client {
    int id;
    char msg[1000000];
}   t_client;

t_client clients[2048];
fd_set  r_set, w_set, master;
int maxfd = 0, gid = 0;
char    s_buf[1000000];
char    r_buf[1000000];

void    err(char *msg) {
    if (msg)
        write(2, msg, strlen(msg));
    else
        write(2, "Fatal error", 11);
    write(2, "\n", 1);
    exit(1);
}

void    send_all(int t_fd) {
    for (int fd = 0; fd <= maxfd; ++fd) {
        if (FD_ISSET(fd, &w_set) && fd != t_fd)
            if (send(fd, s_buf, strlen(s_buf), 0) == -1)
                err(NULL);
    }
}

int main(int argc, char **argv) {
    if (argc != 2)
        err("Wrong number of arguments");

	int sockfd, len;
	struct sockaddr_in servaddr; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
        err(NULL); 
	bzero(&servaddr, sizeof(servaddr));
    maxfd = sockfd;

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1]));

    FD_ZERO(&master);
    FD_SET(sockfd, &master);
    bzero(clients, sizeof(clients));
    bzero(&servaddr, sizeof(servaddr));
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0 
        || listen(sockfd, 100) != 0)
        err(NULL);
}
