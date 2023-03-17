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

void msg_type(char* buff, struct message* msg, struct client* client) {
	if(strncmp(buff,"/nick",5)==0) {
		msg->type = NICKNAME_NEW;
	} else if(strncmp(buff,"/quit",5)==0) {
		msg->type = MULTICAST_QUIT;
	} else if(strncmp(buff,"/who\n",5)==0){
		msg->type = NICKNAME_LIST;
	} else if(strncmp(buff, "/whois", 6)==0){
		msg->type = NICKNAME_INFOS;
	} else if(strncmp(buff, "/msgall",7)==0) {
		msg->type = BROADCAST_SEND;
    } else if(strncmp(buff, "/msg", 4) == 0) {
        msg->type = UNICAST_SEND;
	} else if(strncmp(buff, "/create", 7)==0) {
        msg->type = MULTICAST_CREATE;
    } else if(strncmp(buff, "/channel_list", 13)==0) {
        msg->type = MULTICAST_LIST;
    } else if(strncmp(buff,"/join", 5) == 0) {
        msg->type = MULTICAST_JOIN;
    } else if(strncmp(buff, "/quitall", 8) == 0) {
        msg->type = BROADCAST_QUIT;
    }  else{
        if(client->isinchat == 1) {
            msg->type = MULTICAST_SEND;
        } else {
            msg->type = ECHO_SEND;
        }
	}
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

void echo_client(int sockfd, struct client* client, struct chatrooms* room) {
	struct message msgstruct;
	char buff[MSG_LEN];
	int n;
	char name[NICK_LEN];
	int i, j;
    // Cleaning memory
    memset(&msgstruct, 0, sizeof(struct message));
    memset(buff, 0, MSG_LEN);
    if(strncmp((char*)client->nick,"none",4) != 0){
        strncpy((char *)msgstruct.nick_sender, (char *)client->nick, strlen(client->nick)); 
    }
    // Getting message from client
    n = 0;
    while ((buff[n++] = getchar()) != '\n') {} // trailing '\n' will be sent
    // Filling structure
    msg_type(buff,&msgstruct, client);		//Selecting a message type
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
            char msg[MSG_LEN];
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

        case MULTICAST_CREATE :
            j = 0;
            memset(name, 0,NICK_LEN*sizeof(char));
            for(i=8; i<strlen(buff);i++) {
                name[j] = buff[i];
                j+=1;
            }
            name[j] = '\0';
            strcpy(room->name, name);
            client->isinchat = 1;
            msgstruct.pld_len = strlen(name);
            strcpy(msgstruct.infos, name);
            sending_struct(sockfd, msgstruct);
            break;

        case MULTICAST_LIST :
            msgstruct.pld_len = sizeof(buff);
            sending_struct(sockfd, msgstruct);
            break;

        case MULTICAST_QUIT :
            j = 0;
            memset(name, 0,NICK_LEN*sizeof(char));
            for(i=6; i<strlen(buff);i++) {
                name[j] = buff[i];
                j+=1;
            }
            name[j] = '\0';
            memset(room->name, 0, NICK_LEN);
            msgstruct.pld_len = strlen(name);
            client->isinchat = 0;
            strcpy(msgstruct.infos, name);
            sending_struct(sockfd, msgstruct);
            break;

        case MULTICAST_JOIN :
            j = 0;
            memset(name, 0,NICK_LEN*sizeof(char));
            for(i=6; i<strlen(buff);i++) {
                name[j] = buff[i];
                j+=1;
            }
            name[j] = '\0';
            strcpy(room->name, name);
            msgstruct.pld_len = strlen(name);
            client->isinchat = 1;
            strcpy(msgstruct.infos, name);
            sending_struct(sockfd, msgstruct);
            break;

        case MULTICAST_SEND :
            msgstruct.pld_len = strlen(buff);
            strncpy(msgstruct.nick_sender, client->nick, strlen(client->nick));
            strncpy(msgstruct.infos, room->name, strlen(room->name));
            sending_msg(sockfd, msgstruct, buff);
            break;

        case ECHO_SEND : 
            msgstruct.pld_len = strlen(buff);
            strncpy(msgstruct.nick_sender, client->nick, strlen(client->nick));
            strncpy(msgstruct.infos, "\0", 1);
            sending_msg(sockfd, msgstruct, buff);
            break;

        case BROADCAST_QUIT :
            msgstruct.pld_len = 0;
            strncpy(msgstruct.nick_sender, client->nick, strlen(client->nick));
            sending_struct(sockfd, msgstruct);
            free(client);
            close(sockfd);
            printf("Socket closed\n");
            break;

        default :
            break;
    }

    
    printf("Message sent!\n");
}

void reception(struct message msgstruct, int sockfd, char buff[MSG_LEN]) {
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
	printf("%s\n", buff);
}


int handle_connect(char* addr,char* port) {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(addr, port, &hints, &result) != 0) {
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
    char* addr = argv[1];
    char* port = argv[2];
	struct client* clientici = malloc(sizeof(struct client));
	struct message msgstruct;
    char buff[MSG_LEN];
	strncpy(clientici->nick,init,4);
	sfd = handle_connect(addr, port);
	clientici->sfd = sfd;
    clientici->isinchat = 0;
    struct chatrooms* room = malloc(sizeof(struct chatrooms));
	printf("Connecting to server ... done !\n");
	struct pollfd fds[2];
    fds[0].fd = sfd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
	fds[1].fd = 1;
	fds[1].events = POLLIN;
	fds[1].revents = 0;
	int i;

	printf("The posible request are : /nick 'pseudo', /who, /whois 'pseudo', /msgall 'msg', /quit\n");

    while(1) {
        int r = poll(fds, sizeof(struct pollfd), -1);
        if(r==-1) {
            perror("Poll:");
        }
		for(i=0; i<2; i++) {
			if((fds[i].revents & POLLIN) && (fds[i].fd == fds[0].fd)){
				reception(msgstruct,fds[0].fd,buff);
				fds[0].revents = 0;

			}
			if((fds[i].revents & POLLIN) && (fds[i].fd != fds[0].fd)){
				echo_client(sfd, clientici, room);
				fds[i].revents = 0;
			}
		}
		
	}	
	return EXIT_SUCCESS;
}