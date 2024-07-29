#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

fd_set activeSocks, readSocks, writeSocks;

int maxSocketfd=0;
int ids[100000];
int maxfd = 0;

char write_buff[200000] = {0};
char read_buff[200000] = {0};

void error_exit(const char *msg) 
{
	write(STDERR_FILENO, msg, strlen(msg));
	close(0);
	exit(1);
}

void notify(int who, char *msg)
{
	for (int fd = 0 ; fd <= maxSocketfd; fd++)
		if(FD_ISSET(fd, &writeSocks) && fd != who)
			send(fd, msg, strlen(msg), 0);

}

void add_it(int fd)
{
	maxSocketfd = fd > maxSocketfd ? fd : maxSocketfd;
	ids[fd] = maxfd++;
	FD_SET(fd, &activeSocks);
	sprintf(write_buff, "server: client %d just arrived\n", ids[fd]);
	notify(fd, write_buff);
}

void delete_it(int fd)
{
	sprintf(write_buff, "server: client %d just left\n", ids[fd]);
	notify(fd, write_buff);
	FD_CLR(fd, &activeSocks);
	close(fd);
}


void send_msg(int fd)
{
	char msg[200000] = {0};
	int j = 0;
	int i = 0;

	while(read_buff[i])
	{
		msg[j++] = read_buff[i];
		if(read_buff[i] =='\n' || read_buff[i + 1] == '\0')
		{
			bzero(&write_buff, sizeof(write_buff));
			sprintf(write_buff, "client %d: %s", ids[fd], msg);
			notify(fd, write_buff);
			bzero(&msg, sizeof(msg));
			j = 0;
		}
		i++;
	}
	bzero(&read_buff, sizeof(read_buff));
}

int main(int argc, char* argv[])
{
	if (argc != 2) 
	{
        	error_exit("Wrong number of arguments\n");
	}

	int sockfd, port;
	struct sockaddr_in servaddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		error_exit("Fatal error\n");
	}
	else
	{
		bzero(&servaddr, sizeof(servaddr));
	}

	port = atoi(argv[1]);
	
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(port);
	
	maxSocketfd = sockfd;

	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) 
	{
		error_exit("Fatal error\n");
	}
	if (listen(sockfd, SOMAXCONN) != 0)
	{
		error_exit("fatal error\n");
	}
	FD_ZERO(&activeSocks);
	FD_SET(sockfd, &activeSocks);
	while(42)
	{
		readSocks=writeSocks=activeSocks;
		
		if (select(maxSocketfd + 1, &readSocks, &writeSocks, NULL, NULL) == -1)
			continue;
		for (int fd = sockfd; fd <= maxSocketfd; fd++)
		{
			if (FD_ISSET(fd, &readSocks))
			{
				if (fd == sockfd)
				{
					socklen_t len = sizeof(servaddr);
					int clientSocket = accept(sockfd, (struct sockaddr *)&servaddr, &len);
					if(clientSocket == -1)
						error_exit("Fatal error\n");
					add_it(clientSocket);
					break;
				}
				int bytes_read = 1000;
				while(bytes_read == 1000 || read_buff[strlen(read_buff) - 1] != '\n')
				{
					bytes_read = recv(fd, read_buff + strlen(read_buff), 1000, 0);
					if (bytes_read <= 0)
						break;
				}
				if (bytes_read <= 0)
				{
					send_msg(fd);
					delete_it(fd);
					break;
				}
				send_msg(fd);
			}
		}
	}
return (0);
}

