#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <ctype.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <errno.h>
#include <arpa/inet.h>

#define CL_PORT 6666
#define MAX_CLIENTS 10000
#define BUFLEN 256

int main(void)
{
	int cl_fd;
	int new_sock;
	struct sockaddr_in tcp_addr;
	fd_set read_fds, aux_read;
	struct sockaddr_in client_addr;
	int fdmax, cl_len = sizeof(struct sockaddr_in);
	size_t i;
	int error;

	int tcp_fds[10];
	int tcp_no = 0;

	char buffer[BUFLEN];

	cl_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (cl_fd < 0) {
		perror("Eroare deschidere socket tcp!");
		exit(1);
	}

	/*  Golirea descriptorilor pentru TCP  */
	FD_ZERO(&read_fds);
	FD_ZERO(&aux_read);

	/*  Setare sockaddr_in  */
	memset((char*)&tcp_addr, 0, sizeof(tcp_addr));
	tcp_addr.sin_family = AF_INET;
	tcp_addr.sin_addr.s_addr = INADDR_ANY;
	tcp_addr.sin_port = htons(CL_PORT);

	/*  Bind socketi  */
	if (bind(cl_fd, (struct sockaddr *) &tcp_addr, sizeof(struct sockaddr)) < 0) {
		perror("Eroare bind tcp!");
		exit(1);
	}

	/*  Incepe listen pe MAX_CLIENTS  */
	error = listen(cl_fd, MAX_CLIENTS);
	if(error)
		printf("Eroare listen\n");

	/*  Setare socketi pentru select  */
	FD_SET(0, &read_fds);
	FD_SET(cl_fd, &read_fds);
	fdmax = cl_fd;

	while (1) {
		aux_read = read_fds;

		if (select(fdmax + 1, &aux_read, NULL, NULL, NULL) == -1)
			perror("Error in select!");

		/*  Daca am primit de la stdin verific singurul mesaj posibil -> 'quit'  */
		if(FD_ISSET(0, &aux_read)) {
			fgets(buffer, BUFLEN-1, stdin);
			buffer[strlen(buffer) -1] = '\0';
			for(i = 0; i < tcp_no; i++) {
				error = send(tcp_fds[i], buffer, strlen(buffer), 0);
				if (error < 0)
					printf("Eroare send\n");
			}
		}

		for (i = 1; i<= fdmax; i++) {

			/*  Got connection on the client fd  */
			if (FD_ISSET(i, &aux_read) && i == cl_fd) {
				new_sock = accept(cl_fd,
						(struct sockaddr*) &client_addr,
						&cl_len);
				if (new_sock == -1) {
					perror("Eroare la accept");
				} else {
					tcp_fds[tcp_no++] = new_sock;
					FD_SET(new_sock, &read_fds);
					fdmax = fdmax > new_sock ? fdmax : new_sock;
				}
			}
		}
	}

	close(cl_fd);

	return 0;
}
