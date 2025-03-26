#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct s_client {
    int id;
    char *msg;
} t_client;

t_client clients[FD_SETSIZE];
fd_set master_set, read_set, write_set;
int max_fd = 0, next_id = 0;
char buffer[4096];

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void fatal_error(void) {
    write(2, "Fatal error\n", 12);
    exit(1);
}

void send_all(int except_fd, char *msg) {
    for (int fd = 0; fd <= max_fd; fd++) {
        if (FD_ISSET(fd, &write_set) && fd != except_fd) {
            if (send(fd, msg, strlen(msg), 0) < 0)
                fatal_error();
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        write(2, "Wrong number of arguments\n", 26);
        exit(1);
    }

    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;
    socklen_t len;

    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1)
        fatal_error();

    FD_ZERO(&master_set);
    FD_SET(sockfd, &master_set);
    max_fd = sockfd;

    bzero(&servaddr, sizeof(servaddr)); 
    for (int i = 0; i < FD_SETSIZE; i++)
        clients[i].msg = NULL;

    // assign IP, PORT (portは引数から取得)
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
    servaddr.sin_port = htons(atoi(argv[1])); 

    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0 ||
        listen(sockfd, 10) != 0)
        fatal_error();

    while (1) {
        read_set = write_set = master_set;
        
        if (select(max_fd + 1, &read_set, &write_set, NULL, NULL) < 0)
            fatal_error();
        
        for (int fd = 0; fd <= max_fd; fd++) {
            if (FD_ISSET(fd, &read_set)) {
                if (fd == sockfd) {
                    len = sizeof(cli);
                    connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
                    if (connfd < 0)
                        fatal_error();
                    
                    FD_SET(connfd, &master_set);
                    if (connfd > max_fd)
                        max_fd = connfd;
                    
                    clients[connfd].id = next_id++;
                    clients[connfd].msg = NULL;
                    
                    sprintf(buffer, "server: client %d just arrived\n", clients[connfd].id);
                    send_all(connfd, buffer);
                } else {
                    char recv_buf[4096];
                    int bytes = recv(fd, recv_buf, sizeof(recv_buf) - 1, 0);
                    
                    if (bytes <= 0) {
                        sprintf(buffer, "server: client %d just left\n", clients[fd].id);
                        send_all(fd, buffer);
                        
                        FD_CLR(fd, &master_set);
                        close(fd);
                        if (clients[fd].msg)
                            free(clients[fd].msg);
                        clients[fd].msg = NULL;
                    } else {
                        recv_buf[bytes] = '\0';
                        clients[fd].msg = str_join(clients[fd].msg, recv_buf);
                        
                        char *msg = NULL;
                        while (extract_message(&clients[fd].msg, &msg) == 1) {
                            sprintf(buffer, "client %d: %s", clients[fd].id, msg);
                            send_all(fd, buffer);
                            free(msg);
                            msg = NULL;
                        }
                        
                        if (msg) free(msg);
                    }
                }
            }
        }
    }
    
    return 0;
}