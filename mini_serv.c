#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct s_client
{
	int		id;
	char	msg[1000000];
}	t_client;

t_client	clients[2048];
fd_set		w_set, r_set, master;
int			maxfd = 0, gid = 0;
char		s_buf[1000000], r_buf[1000000];

void	err(char *msg)
{
	if (msg)
		write(2, msg, strlen(msg));
	else
		write(2, "Fatal error", 11);
	write(2, "\n", 1);
	exit(1);
}

void	send_to_all(int except_fd)
{
	for (int fd = 0; fd <= maxfd; fd++)
	{
		if (FD_ISSET(fd, &w_set) && fd != except_fd)
			if (send(fd, s_buf, strlen(s_buf), 0) == -1)
				err(NULL);
	}
}

int	main(int argc, char **argv)
{
	if (argc != 2)
		err("Wrong number of arguments");

	struct sockaddr_in	servaddr;
	socklen_t			len;

	int	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) err(NULL);
	maxfd = sockfd;

	FD_ZERO(&master);
	FD_SET(sockfd, &master);
	bzero(clients, sizeof(clients));
	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));

	if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) != 0
		|| listen(sockfd, 100) != 0)
		err(NULL);

	while (1)
	{
		r_set = w_set = master;
		if (select(maxfd + 1, &r_set, &w_set, 0, 0) < 0)
			err(NULL);

		for (int fd = 0; fd <= maxfd; fd++)
		{
			if (FD_ISSET(fd, &r_set))
			{
				if (fd == sockfd)
				{
					int clientfd = accept(sockfd, (struct sockaddr *)&servaddr, &len);
					if (clientfd == -1) err(NULL);
					if (clientfd > maxfd) maxfd = clientfd;
					FD_SET(clientfd, &master);
					clients[clientfd].id = gid++;
					sprintf(s_buf, "server: client %d just arrived\n", clients[clientfd].id);
					send_to_all(clientfd);
					bzero(s_buf, sizeof(s_buf));
				}
				else
				{
					int ret = recv(fd, r_buf, sizeof(r_buf), 0);
					if (ret <= 0)
					{
						sprintf(s_buf, "server: client %d just left\n", clients[fd].id);
						send_to_all(fd);
						bzero(s_buf, sizeof(s_buf));
						FD_CLR(fd, &master);
						bzero(clients[fd].msg, sizeof(clients[fd].msg));
						close(fd);
					}
					else
					{
						for (int i = 0, j = strlen(clients[fd].msg); i < ret; i++, j++)
						{
							clients[fd].msg[j] = r_buf[i];
							if (clients[fd].msg[j] == '\n')
							{
								clients[fd].msg[j] = '\0';
								sprintf(s_buf, "client %d: %s\n", clients[fd].id, clients[fd].msg);
								send_to_all(fd);
								bzero(s_buf, sizeof(s_buf));
								bzero(clients[fd].msg, sizeof(clients[fd].msg));
                                j = -1;
							}
						}
					}
				}
				break ;
			}
		}
	}
	return (0);
}