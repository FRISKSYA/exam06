#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct s_client {
    int id;
    char *msg;
}   t_client;

t_client clients[FD_SETSIZE];
fd_set  master_set, read_set, write_set;
int max_fd = 0, next_id = 0;

// ====== COPY FROM MAIN =====
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

// ===== END =====

void    fatal_error()
{
    write(2, "Fatal error\n",12);
    exit(1);
}

void    send_all(int except_fd, char *msg)
{
    for (int fd = 0; fd <= max_fd; ++fd)
    {
        if (FD_ISSET(fd, &write_set) && fd != except_fd)
        {
            if (send(fd, msg, strlen(msg), 0) < 0)
                fatal_error();
        }
    }
}

void    handle_new_connection(int server_fd)
{
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd < 0)
        fatal_error();

    FD_SET(client_fd, &master_set);
    if (client_fd > max_fd)
        max_fd = client_fd;

    clients[client_fd].id = next_id++;
    clients[client_fd].msg = NULL;

    char    buffer[4096];
    sprintf(buffer, "server: client %d just arrived\n", clients[client_fd].id);
    send_all(client_fd, buffer);
}

void    handle_client_message(int client_fd)
{
    char    recv_buf[4096];
    int     bytes = recv(client_fd, recv_buf, sizeof(recv_buf) - 1, 0);

    if (bytes <= 0)
    {
        char    buffer[4096];
        sprintf(buffer, "server: client %d just left\n", clients[client_fd].id);
        send_all(client_fd, buffer);

        FD_CLR(client_fd, &master_set);
        close(client_fd);
        if (clients[client_fd].msg)
            free(clients[client_fd].msg);
        clients[client_fd].msg = NULL;
    }
    else
    {
        recv_buf[bytes] = '\0';
        clients[client_fd].msg = str_join(clients[client_fd].msg, recv_buf);

        char    *msg = NULL;
        while (extract_message(&clients[client_fd].msg, &msg) == 1)
        {
            char    buffer[4096];
            sprintf(buffer, "client %d: %s", clients[client_fd].id, msg);
            send_all(client_fd, buffer);
            free(msg);
        }
    }
}

int main(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in servaddr; 

    if (argc != 2)
    {
        write(2, "Wrong number of arguments\n", 26);
        exit(1);
    }

    // ===== COPY and FIX FROM MAIN =====
	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) 
        fatal_error();
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) 
        fatal_error();
	if (listen(sockfd, SOMAXCONN) != 0) 
        fatal_error();

    // ===== END =====

    FD_ZERO(&master_set);
    FD_SET(sockfd, &master_set);
    max_fd = sockfd;

    for (int i = 0; i < FD_SETSIZE; ++i)
        clients[i].msg = NULL;

    while (1)
    {
        read_set = write_set = master_set;

        if (select(max_fd + 1, &read_set, &write_set, NULL, NULL) < 0)
            fatal_error();
        
        for (int fd = 0; fd <= max_fd; ++fd)
        {
            if (FD_ISSET(fd, &read_set))
            {
                if (fd == sockfd)
                    handle_new_connection(sockfd);
                else
                    handle_client_message(fd);
            }
        }
    }
    return (0);
}
