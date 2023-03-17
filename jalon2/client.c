#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

#include "common.h"
#include "msg_struct.h"

int msg_type(char* buff, struct message* msg) {
	if(strncmp(buff,"/nick",5)==0) {
		msg->type = NICKNAME_NEW;
	} else if(strncmp(buff,"/quit\n",7)==0) {
		msg->type = MULTICAST_QUIT;
	} else if(strncmp(buff,"/who\n",5)==0){
		msg->type = NICKNAME_LIST;
	} else if(strncmp(buff, "/whois", 6)==0){
		msg->type = NICKNAME_INFOS;
	} else if(strncmp(buff, "/msgall",7)==0) {
		msg->type = BROADCAST_SEND;
	} else if(strncmp(buff, "/msg", 4) == 0) {
		msg->type = UNICAST_SEND;
	} else{
		msg->type = ECHO_SEND;
	}

	return 0;
}

void sending_struct(int sockfd, struct message msg) {
	// Sending structure
	send(sockfd, &msg, sizeof(msg), 0);
}

void sending_msg(int sockfd, struct message msg, char buff[MSG_LEN]) {
	// Sending structure
	send(sockfd, &msg, sizeof(msg), 0);
	// Sending message
	send(sockfd, buff, msg.pld_len, 0);
}

void echo_client(int sockfd, struct client* client, int* quit) {
	struct message msgstruct;
	char buff[MSG_LEN];
	int n;
	char name[NICK_LEN];
	int i, j;
	char msg[MSG_LEN];
		// Cleaning memory
		memset(&msgstruct, 0, sizeof(struct message));
		memset(buff, 0, MSG_LEN);
		if(strncmp((char*)client->nick,"none",4) != 0){
			strncpy((char *)msgstruct.nick_sender, (char *)client->nick, strlen(client->nick)); 
		}
		// Getting message from client
		//printf("Message: ");
		n = 0;
		while ((buff[n++] = getchar()) != '\n') {} // trailing '\n' will be sent
		printf("buff in echo client : %s\n", buff);
		// Filling structure
		int res = msg_type(buff,&msgstruct);		//Selecting a message type
		if(res == 100) {
			msgstruct.pld_len = strlen(buff);
			if (send(sockfd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
				//break;
			}
			// Sending message
			if (send(sockfd, buff, msgstruct.pld_len, 0) <= 0) {
				//break;
			}
			close(sockfd);
			printf("Socket closed\n");
			//break;
		}
		switch(msgstruct.type){			//Filling structure according to message type
			case NICKNAME_NEW :
				j = 0;
				char* tmp = strchr(buff,'\n');
				*tmp = '\0';
				memset(client->nick, 0, NICK_LEN*sizeof(char));
				memset(msgstruct.nick_sender, 0, NICK_LEN*sizeof(char));
				memset(name, 0,NICK_LEN*sizeof(char));
				for(i=6; i<strlen(buff);i++) {
					name[j] = buff[i];
					j+=1;
				}
				name[j] = '\0';
				msgstruct.pld_len = strlen(name);
				strcpy(msgstruct.infos, name);
				strncpy(msgstruct.nick_sender, name, strlen(name));
				strncpy(client->nick, name, strlen(name));
				sending_struct(sockfd, msgstruct);
				break;
			
			case NICKNAME_LIST :
				msgstruct.pld_len = sizeof(buff);
				sending_struct(sockfd, msgstruct);
				break;
			
			case NICKNAME_INFOS :
				j = 0;
				memset(name, 0,NICK_LEN*sizeof(char));
				for(i=6; i<strlen(buff);i++) {
					name[j] = buff[i];
					j+=1;
				}
				name[j] = '\0';
				msgstruct.pld_len = strlen(name);
				strcpy(msgstruct.infos, name);
				sending_struct(sockfd, msgstruct);
				break;
			
			case BROADCAST_SEND : ;
				j = 0;
				memset(msg, 0,MSG_LEN*sizeof(char));
				for(i=8; i<strlen(buff);i++) {
					msg[j] = buff[i];
					j+=1;
				}
				msgstruct.pld_len = strlen(msg);
				strcpy(msgstruct.infos, msg);
				strncpy(msgstruct.nick_sender, client->nick, strlen(client->nick));
				sending_struct(sockfd, msgstruct);
				break;

			case UNICAST_SEND :
				i = 5;
				j = 0;
				memset(name, 0,NICK_LEN*sizeof(char));
				while(buff[i] != ' '){
					name[j] = buff[i];
					i+=1;
					j+=1;
				}
				name[j] = '\0';
				strcpy(msgstruct.infos, name);
				j = 0;
				i++;
				while(i<strlen(buff)) {
					msg[j] = buff[i];
					i++;
					j++;
				}
				msg[j] = '\0';
				msgstruct.pld_len = strlen(msg);
				sending_msg(sockfd, msgstruct, msg);
				break;

			case ECHO_SEND : 
				msgstruct.pld_len = strlen(buff);
				strncpy(msgstruct.nick_sender, client->nick, strlen(client->nick));
				strncpy(msgstruct.infos, "\0", 1);
				sending_msg(sockfd, msgstruct, buff);
				break;

			case MULTICAST_QUIT :
				msgstruct.pld_len = 0;
				strncpy(msgstruct.nick_sender, client->nick, strlen(client->nick));
				sending_struct(sockfd, msgstruct);
				free(client);
				*quit = 100;
				break;

			default :
				break;
		}

		
		printf("Message sent!\n");
}

void reception(int sockfd, char buff[MSG_LEN]) {
	struct message msgstruct;
	// Cleaning memory
	memset(&msgstruct, 0, sizeof(struct message));
	memset(buff, 0, MSG_LEN);
	// Receiving structure
	if (recv(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
		//break;
	}
	// Receiving message
	if (recv(sockfd, buff, msgstruct.pld_len, 0) <= 0) {
		//break;
	}
	printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
	printf("Received: %s\n", buff);
}


int handle_connect() {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(SERV_ADDR, SERV_PORT, &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,rp->ai_protocol);
		if (sfd == -1) {
			continue;
		}
		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
			break;
		}
		close(sfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "Could not connect\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result);
	return sfd;
}

int main(int argc, char* argv[]) {
	int sfd;
	char* init = "none";
	struct client* clientici = malloc(sizeof(struct client));
	strncpy(clientici->nick,init,4);
	sfd = handle_connect();
	clientici->sfd = sfd;
	int quit = 0;
	char buff[MSG_LEN];
	printf("Connecting to server ... done !\n");
	struct pollfd fds[2];
    fds[0].fd = sfd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
	fds[1].fd = 1;
	fds[1].events = POLLIN;
	fds[1].revents = 0;
	int i;

	printf("Press enter before typing a request\n");
	printf("The posible request are : /nick 'pseudo', /who, /whois 'pseudo', /msgall 'msg', /quit\n");

    while(1) {
        int r = poll(fds, sizeof(struct pollfd), -1);
        if(r==-1) {
            perror("Poll:");
        }
		for(i=0; i<2; i++) {
			if((fds[i].revents & POLLIN) && (fds[i].fd == fds[0].fd)){
				reception(fds[0].fd,buff);
				fds[0].revents = 0;

			}
			if((fds[i].revents & POLLIN) && (fds[i].fd != fds[0].fd)){
				printf("on veut envoyer qqc\n");
				echo_client(sfd, clientici, &quit);
				fds[i].revents = 0;
			}
			if(quit == 100){
				break;
			}
		}
		if(quit == 100) {
			break;
		}
		
	}	
	return EXIT_SUCCESS;
}