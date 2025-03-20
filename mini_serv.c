#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/ip.h>

int	count = 0, max_fd = 0;
int	ids[65536];
char	*msgs[65536];

fd_set	rfds, wfds, afds;
char	buf_read[1001], buf_write[42];

int	extract_message(char **buf, char **msg);

char	*str_join(char *buf, char *add);

void	fatal_error();

void	notify_other(int author, char *str);

void	register_client(int fd);

void	remove_client(int fd);

void	send_msg(int fd);

int		create_socket();

int	main(int argc, char **argv)
{
	if (ac != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	FD_ZERO(&afds);
	int sockfd = create_socket();

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433);
	servaddr.sin_port = htons(atoi(argv[1]));

	if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)))
		fatal_error();
	if (listen(sockfd, SOMAXCONN))
		fatal_error();

	while (1)
	{
		rfds = wfds = afds;

		if (select(max_fd + 1, &rfds, &wfds, NULL, NULL) < 0)
			fatal_error();

		for (int fd = 0; fd <= max_fd; ++fd)
		{
			if (!FD_ISSET(fd, &rfds))
				continue;

			if (fd == sockfd)
			{
				socklen_t addr_len = sizeof(servaddr);
				int client_fd = accept(sockfd, (struct sockaddr *)&servaddr, &addr_len);
				if (clinet_fd >= 0)
				{
					register_client(client_fd);
					break ;
				}
			}
			else
			{
				int	read_bytes = recv(fd, buf_read, 1000, 0);
				if (read_bytes <= 0)
				{
					remove_client(fd);
					break ;
				}
				buf_read[read_bytes] = '\0';
				msgs[fd] = str_join(msgs[fd], buf_read);
				send_msg(fd);
			}
		}
	}
	return 0;
}
