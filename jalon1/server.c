#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <err.h>

#include "common.h"
#define NBR_CO 10

struct client* init(int sfd, uint32_t addr, u_short port) {
	struct client* client = malloc(sizeof(struct client*));
	client->sfd = sfd;
	client->ADDR = addr;
	client->PORT = port;
	client->next = NULL;

	return client;
}

void echo_server(int sockfd, short* events) {
	char buff[MSG_LEN];
	char* fin = "/quit";
	while (1) {
		// Cleaning memory
		memset(buff, 0, MSG_LEN);
		// Receiving message
		if (recv(sockfd, buff, MSG_LEN, 0) <= 0) {
			break;
		}
		printf("Received: %s", buff);
		if(strncmp(buff,fin,5)==0) {
			*events = POLLHUP;
			printf("Client wants to disconnect\n");
		}
		// Sending message (ECHO)
		if (send(sockfd, buff, strlen(buff), 0) <= 0) {
			break;
		}
		printf("Message sent!\n");
		break;
	}
}

int handle_bind(const char* SERV_PORT) {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(NULL, SERV_PORT, &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,
		rp->ai_protocol);
		if (sfd == -1) {
			continue;
		}
		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
			break;
		}
		close(sfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "Could not bind\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result);
	return sfd;
}

int main(int argc, char* argv[]) {
	struct sockaddr cli;
	int sfd, connfd;
    const char* SERV_PORT = argv[1];
	socklen_t len;
	struct start_client start;
	start.first = NULL;
	sfd = handle_bind(SERV_PORT);
	if ((listen(sfd, SOMAXCONN)) != 0) {
		perror("listen()\n");
		exit(EXIT_FAILURE);
	}
	len = sizeof(cli);
    struct pollfd fds[NBR_CO];
    fds[0].fd = sfd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    int i;
    for(i=1; i<NBR_CO; i++) {
        fds[i].fd = -1;
        fds[i].events = 0;
        fds[i].revents = 0;
    }

    while(1) {
        int r = poll(fds, sizeof(struct pollfd), -1);
        if(r==-1) {
            perror("Poll:");
        }

        for(i=0; i<NBR_CO; i++) {
            if((fds[i].revents & POLLIN) && (fds[i].fd == fds[0].fd)) {
                struct sockaddr_in client_addr;
                memset(&client_addr, 0, sizeof(client_addr));
                int addrlen = sizeof(client_addr);
                if((connfd = accept(sfd, (struct sockaddr*) &client_addr,&addrlen))<0){
                    perror("accept()\n");
                    exit(EXIT_FAILURE);
                }
				if(start.first == NULL) {
					start.first = init(connfd,(client_addr.sin_addr).s_addr,client_addr.sin_port);
				} else {
					struct client* ptr = start.first;
					while (ptr->next != NULL) {
						ptr = ptr->next;
					}
					ptr->next = init(connfd,(client_addr.sin_addr).s_addr,client_addr.sin_port);
				}
                int j = 1;
                while(fds[j].fd != -1) {
                    j ++;
                }
                fds[j].fd = connfd;
                fds[j].events =POLLIN;
                fds[j].revents = 0;
                fds[0].revents = 0;
            }
            if((fds[i].revents & POLLIN) && (fds[i].fd != fds[0].fd)) {
                fds[i].revents = 0;
                echo_server(fds[i].fd,&(fds[i].revents));
            }
            if((fds[i].revents & POLLHUP) && (fds[i].fd != fds[0].fd)) {
                int fin;
				struct client* ptract = start.first;
				struct client* ptrsave = NULL;
				while((ptract->next != NULL)) {
					if(ptract->sfd == fds[i].fd) {
						break;
					} else if((ptract == start.first) && (ptract->next == NULL)) {
					start.first = NULL;
					} else {
						ptrsave = ptract;
						ptract = ptract->next;
					}
				}
                fin=close(fds[i].fd);
                if(fin == -1) {
                    perror("close:\n");
                }
                fds[i].fd = -1;
                fds[i].events = 0;
                fds[i].revents = 0;

            }
        }
    }
	
	close(sfd);
	return EXIT_SUCCESS;
}

