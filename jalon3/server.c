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
#include "msg_struct.h"

void init(struct client* newclient, int sfd, uint32_t addr, u_short port, int id) { //Initialisation du client
	newclient->client_id = id;
	newclient->sfd = sfd;
	newclient->ADDR = addr;
	newclient->PORT = port;
    newclient->chat = NULL;
	newclient->next = NULL;
}

int nickname(char name[]) {     //Vérification des règles du surnom
	int i, lengthname, j;
	lengthname = strlen(name);
	if(lengthname > NICK_LEN) {
		return 2;
	}
	for(i=0; i<lengthname-2;i++) {
		if(((name[i]>47) && (name[i]<58)) || ((name[i]>64)&&(name[i]<91)) || ((name[i]>96)&&(name[i]<123))) {
			j++;
		} else {
			return 1;
		}
	}
	return 0;
}

int checkname(struct start_client start, char name[]) { //Vérification de la disponibilité d'un nom pour un client
	if(start.first != NULL) {
		struct client* clientliste = start.first;
		while(clientliste->next != NULL) {
			if(strncmp(clientliste->nick,name,strlen(name))==0) {
				return 100;
			} else {
				clientliste = clientliste->next;
			}
			
		}
	}
	return 0;
}

int checkservname(struct chatrooms chats[], char name[]) {  //Vérification de la disponibilité d'un nom pour un salon
    int i;
	for(i=0;i<CHAT_LEN; i++) {
        if(strncmp(chats[i].name,name,strlen(name))==0) {
            return 100;
        }
	}
	return 0;
}

void userlist(struct start_client start, char buff[]) {			//list of connected users
	if(start.first != NULL) {
		struct client* clientliste = start.first;
		strcpy((char*)buff, "Online Users :");
		strcat(buff," - ");
		strcat(buff,clientliste->nick);
		strcat(buff,"\n");
		while(clientliste->next != NULL) {
			clientliste = clientliste->next;
			strcat(buff,"-");
			strcat(buff,clientliste->nick);
			strcat(buff,"\n");
		}
	}
}

void sending(int sockfd, struct message msgstruct, char buff[]) {
	send(sockfd, &msgstruct, sizeof(msgstruct), 0);	//sending structure
	send(sockfd, buff, msgstruct.pld_len, 0);	//sending message
}

struct client* researchbyname(struct start_client start, char name[]) { //Recherche d'un client dans la liste chaînée à partir de son nom
	struct client* client = start.first;
	int i=0;
	while(1) {
		if(strlen(client->nick)==strlen(name)-1) {
			while(i<strlen(name)-1){
				if(name[i] == client->nick[i]) {
					i ++;
				} else {
					i = 0;
					break;
				}
			}
            if(i != 0) {
                return client;
            }
		}
		if(client->next != NULL){
			client = client->next;
		} else {
			break;
		}
	}
	return NULL;
}

void broadsend(struct start_client start, char buff[], char name[], struct message msgstruct) { //Envoi d'un nmessage en broadcast
	struct client* clientenv = malloc(sizeof(struct client));
	clientenv = start.first;
	while(clientenv->next != NULL) {
		if(strcmp(name, clientenv->nick)!=0) {
			sending(clientenv->sfd, msgstruct, buff);
		}
		clientenv = clientenv->next;
	}
}

void unisend(char name[], struct start_client start, struct message msgstruct, char envoi[], int sockfd) {  //Envoi d'un message en unicast
	struct client* clientenv = researchbyname(start, name);
	if(clientenv == NULL){
		char err[] = "[Server] Unreachable client, please check the user name\n";
		msgstruct.pld_len = strlen(err);
		sending(sockfd, msgstruct, err);
	}
	sending(clientenv->sfd, msgstruct, envoi);
	free(clientenv);
}

char* getinfos(char name[], struct start_client start,char* buff) {     //Récupération des informations d'un client donné à aprtir de son nom
	struct client* clientliste = researchbyname(start, name);
	char text[MSG_LEN];

	memset(buff, 0, MSG_LEN);
	strcpy(buff, "[Server] : ");
	strcat(buff,clientliste->nick);

	strcat(buff," is connected from IP address ");
	memset(text, 0, MSG_LEN);
	sprintf(text, "%d", clientliste->ADDR);
	strcat(buff,text);

	strcat(buff," and from port ");
	memset(text, 0, MSG_LEN);
	sprintf(text, "%d", clientliste->PORT);
	strcat(buff,text);

	strcat(buff,"\n");
	return buff;
}

void creatingsalon(struct chatrooms* chats[], char name[]) {  //Création d'un salon
    int i = 0;
    while((chats[i]->server_id != -1) && (i<CHAT_LEN)){
        i ++;
    }
    chats[i]->server_id = i;
    strcpy(chats[i]->name, name);
}

struct chatrooms* searchroom(struct chatrooms* chats[], char name[]) {  //Recherhce de l'adresse d'un salon à partir de son nom
    int i = 0;
    while((strncmp(chats[i]->name,name, strlen(name)) !=0) && (i<CHAT_LEN)){
        i ++;
    }
    if(i==CHAT_LEN) {
        return NULL;
    }
    return chats[i];
}

void adduser(struct chatrooms* room, struct client* client) {  //Ajout d'un client dans un salon
    client->chat = room;
    int i;
    for(i=0;i<USR_CHAT;i++) {
        if(room->users[i] == NULL) {
            room->users[i] = client;
            break;
        }
    }
}

void quitroom(struct chatrooms* room, struct client* client) {  //Départ d'un client d'un salon
    int i;
    for(i=0;i<USR_CHAT; i++) {
        printf("%d",i);
        if(room->users[i] != NULL) {
            if((room->users[i])->sfd==client->sfd) {
                printf("found\n");
                room->users[i] = NULL;
                break;
            }
        }
    }
    client->chat = NULL;
}

void roomlist(struct chatrooms chats[], char buff[]) {  //Liste des noms des salons existants
    int i;
    strcpy(buff,"List of existing chatrooms : \n");
    for(i=0; i<CHAT_LEN; i++) {
        if(chats[i].server_id != -1) {
            strcat(buff, "- ");
            strcat(buff, chats[i].name);
            strcat(buff, "\n");
        }  
    }
}

void sendinginchat(char buff[], struct chatrooms* room, struct client* sender, struct message* msgstruct) {  //Envoi d'un message aux membres d'un salon
    int i;
    for(i=0;i<USR_CHAT;i++) {
        if(room->users[i] != NULL) {
            if((room->users[i])->sfd != sender->sfd) {
                printf("sending\n");
                sending((room->users[i])->sfd, *msgstruct, buff);
            }
        }
    }
}


void echo_server(int sockfd, struct client* clientact, short* revents, struct start_client start, struct chatrooms chats[]) { //Protocole d'actions en focntion du type de message reçu
    struct message msgstruct;
    char buff[MSG_LEN];
    char name[NICK_LEN];
    char text[MSG_LEN];
    int i, j, g, r;
    char err1[] = "[Server] Unauthorized character used for the room's name\n";
    char err2[] = "[Server] The nickname is too long, please retry with a nickname under 128 characters\n";
    char err3[] = "[Server] This nickname is already used\n";
    char success[] = "[Server] Nickname accepted\n";
    struct chatrooms* room;
    while (1) {
        // Cleaning memory
        memset(&msgstruct, 0, sizeof(struct message));
        memset(buff, 0, MSG_LEN);
        memset(name, 0, sizeof(char)*NICK_LEN);
        // Receiving structure
        if (recv(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
            break;
        }
        switch(msgstruct.type) {
            case NICKNAME_NEW : ;
                j = 0;
                for(i=0; i<strlen(msgstruct.infos);i++) {
                    name[j] = msgstruct.infos[i];
                    j+=1;
                }
                g = checkname(start, name);
                if(g==100) {
                    msgstruct.pld_len = strlen(err3);
                    sending(sockfd, msgstruct,err3);
                    break;
                }
                r = nickname(name);			//checking nickname
                if(r==0){						//Correct nickname
                    memset(clientact->nick, 0, NICK_LEN*sizeof(char));
                    strcpy(clientact->nick,name);
                    msgstruct.pld_len = strlen(success);
                    sending(sockfd, msgstruct, success);
                } else if(r==1) {								//character error
                    msgstruct.pld_len = strlen(err1);
                    sending(sockfd, msgstruct,err1);
                } else if(r==2) {								//length error
                    msgstruct.pld_len = strlen(err2);
                    sending(sockfd, msgstruct,err2);
                }
                printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
                printf("Received: %s\n", buff);
                break;

            case NICKNAME_LIST :
                printf("client name : %s\n", clientact->nick);
                printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
                userlist(start,buff);
                msgstruct.pld_len = sizeof(buff);
                sending(sockfd, msgstruct, buff);
                break;

            case NICKNAME_INFOS :
                j = 0;
                for(i=1; i<strlen(msgstruct.infos);i++) {
                    name[j] = msgstruct.infos[i];
                    j+=1;
                }
                getinfos(name, start, buff);
                msgstruct.pld_len = strlen(buff);
                sending(sockfd, msgstruct, buff);
                break;

            case BROADCAST_SEND :
                printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
                char envoi[MSG_LEN];
                sprintf(text,"%s", msgstruct.nick_sender);
                strcpy(envoi, "[user");
                strcat(envoi,text);
                strcat(envoi,"] ");
                strcat(envoi, msgstruct.infos);
                msgstruct.pld_len = strlen(envoi);
                broadsend(start,envoi, name, msgstruct);
                break;

            case UNICAST_SEND :
				// Receiving message
				if (recv(sockfd, buff, msgstruct.pld_len, 0) < 0) {
					break;
				}
                printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
				strcpy(name, msgstruct.infos);
				sprintf(text,"%s", msgstruct.nick_sender);
				strcpy(envoi, "[Private chat : user");
				strcat(envoi,text);
				strcat(envoi,"] ");
				strcat(envoi, buff);
                msgstruct.pld_len = strlen(envoi);
				unisend(name, start, msgstruct, envoi,sockfd);
				break;

            case MULTICAST_CREATE :
                j = 0;
                for(i=0; i<strlen(msgstruct.infos);i++) {
                    name[j] = msgstruct.infos[i];
                    j+=1;
                }
                g = checkservname(chats, name);
                if(g==100) {
                    msgstruct.pld_len = strlen(err3);
                    sending(sockfd, msgstruct,err3);
                    break;
                }
                r = nickname(name);			//checking nickname
                if(r==0){						//Correct nickname
                    if(clientact->chat != NULL) {
                        quitroom(clientact->chat, clientact);
                    }
                    creatingsalon(&chats, name);
                    room = searchroom(&chats, name);
                    adduser(room, clientact);
                    msgstruct.pld_len = strlen(success);
                    sending(sockfd, msgstruct, success);
                } else if(r==1) {								//character error
                    msgstruct.pld_len = strlen(err1);
                    sending(sockfd, msgstruct,err1);
                } else if(r==2) {								//length error
                    msgstruct.pld_len = strlen(err2);
                    sending(sockfd, msgstruct,err2);
                }
                
                printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
                break;

            case MULTICAST_LIST :
                printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
                roomlist(chats, buff);
                msgstruct.pld_len = sizeof(buff);
                sending(sockfd, msgstruct, buff);
                break;
            
            case MULTICAST_QUIT :
                j = 0;
                printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
                for(i=0; i<strlen(msgstruct.infos);i++) {
                    name[j] = msgstruct.infos[i];
                    j+=1;
                }
                room = searchroom(&chats, name);
                printf("byye\n");
                quitroom(room, clientact);
                printf("ici\n");
                j = 0;
                for(i=0; i<USR_CHAT; i++) {
                    if(room->users[i]!=NULL) {
                        j ++;
                    }
                }
                printf("%d\n", j);

                if(j==0) {
                    room->server_id = -1;
                    memset(room->name, 0, NICK_LEN);
                    char bye[MSG_LEN] = "[Server] You were the last user of this chat, this salon is now destroyed\n";
                    msgstruct.pld_len = strlen(bye);
                    sending(sockfd, msgstruct, bye);
                } else {
                    char bye[MSG_LEN] = "[Server] ";
                    sprintf(text, "%s ", clientact->nick);
                    strcat(bye, text);
                    strcat(bye, "left the chat\n");
                    msgstruct.pld_len = strlen(bye);
                    sendinginchat(bye, room, clientact, &msgstruct);
                }
                break;

            case MULTICAST_JOIN :
                j = 0;
                for(i=0; i<strlen(msgstruct.infos);i++) {
                    name[j] = msgstruct.infos[i];
                    j+=1;
                }
                if(clientact->chat != NULL) {
                    quitroom(clientact->chat, clientact);
                }
                room = searchroom(&chats, name);
                adduser(room, clientact);
                clientact->chat = room;
                char hello[MSG_LEN] = "[Server] ";
                sprintf(text, "%s ", clientact->nick);
                strcat(hello, text);
                strcat(hello, "joined the chat\n");
                msgstruct.pld_len = strlen(hello);
                sendinginchat(hello, room, clientact, &msgstruct);
                break;

            case MULTICAST_SEND :
                printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
                printf("Received: %s\n", buff);
                sendinginchat(buff, clientact->chat, clientact, &msgstruct);
                break;

            case ECHO_SEND :
                // Receiving message
                if (recv(sockfd, buff, msgstruct.pld_len, 0) < 0) {
                    break;
                }
                printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
                printf("Received: %s\n", buff);
                sending(sockfd, msgstruct,buff);
                printf("Message sent!\n");
                break;
            case BROADCAST_QUIT :
                *revents = POLLHUP;
                printf("Client wants to disconnect\n");
                break;

            default : 
                break;
        }
        break;

    }
}



int handle_bind(char* port) {  //Gestion ouverture de la listen socket
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(NULL, port, &hints, &result) != 0) {
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

void release(struct start_client start, int fds) { //Procédure de départ d'un client, déconnexion du serveur global
	struct client* ptract = start.first;
	struct client* ptrsave = start.first;
	while((ptract->sfd != fds) || (ptract->next != NULL)) {
		ptrsave = ptract;
		ptract = ptract->next;
		
	}
	if(ptract == start.first) {
			if(ptract->next == NULL) {
				free(ptract);
				start.first = NULL;
			} else {
				start.first = ptract->next;
				free(ptract);
			}
	} else {
		if(ptract->next != NULL) {
			ptrsave->next = ptract->next;
			free(ptract);
		} else {
			free(ptract);
			ptrsave->next = NULL;
		}
	}
}

int main(int argc, char* argv[]) {
	struct sockaddr cli;
    char* port = argv[1];
	int sfd, connfd, nbrclient;
	socklen_t len;
	struct start_client start;
	start.first = NULL;
	nbrclient = 0;
	sfd = handle_bind(port);
	if ((listen(sfd, SOMAXCONN)) != 0) {
		perror("listen()\n");
		exit(EXIT_FAILURE);
	}
	len = sizeof(cli);
    len = sizeof(cli);
    struct pollfd fds[NBR_CO];
    fds[0].fd = sfd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    int i,j;
    for(i=1; i<NBR_CO; i++) {
        fds[i].fd = -1;
        fds[i].events = 0;
        fds[i].revents = 0;
    }
    struct chatrooms chats[CHAT_LEN];
    for(i=0; i<CHAT_LEN; i++) {
        chats[i].server_id = -1;
        for(j=0; j<USR_CHAT;j++) {
            chats[i].users[j] = NULL;
        }
    }

    while(1) {
        int r = poll(fds, sizeof(struct pollfd), -1);
        if(r==-1) {
            perror("Poll:");
        }

        for(i=0; i<NBR_CO; i++) {
            if((fds[i].revents & POLLIN) && (fds[i].fd == fds[0].fd)) { //Demande de connexion
                struct sockaddr_in client_addr;
                memset(&client_addr, 0, sizeof(client_addr));
                int addrlen = sizeof(client_addr);
				short* help;
                if((connfd = accept(sfd, (struct sockaddr*)&client_addr,&addrlen))<0){
                    perror("accept()\n");
                    exit(EXIT_FAILURE);
                }
				nbrclient ++;
				if(start.first == NULL) {
					start.first = malloc(sizeof(struct client));
					memset(start.first, 0, sizeof(struct client));
					init(start.first, connfd,(client_addr.sin_addr).s_addr,client_addr.sin_port,nbrclient);
					echo_server(connfd, start.first, help, start,chats);
				} else {
					struct client* ptr = start.first;
					while (ptr->next != NULL) {
						ptr = ptr->next;
					}
					ptr->next = malloc(sizeof(struct client));
					ptr = ptr->next;
					memset(ptr, 0, sizeof(struct client));
					init(ptr,connfd,(client_addr.sin_addr).s_addr,client_addr.sin_port,nbrclient);
					echo_server(connfd, ptr, help, start, chats);
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
				struct client* cliact = start.first;
				while((cliact->sfd != fds[i].fd)) {
					if(cliact->next != NULL) {
						cliact = cliact->next;
						break;
					}
				}
                echo_server(fds[i].fd, cliact, &(fds[i].revents), start, chats);	//&(fds[i].revents)
            }

            if((fds[i].revents & POLLHUP) && (fds[i].fd != fds[0].fd)) {
                int fin;
				struct client* ptract = start.first;
				release(start, fds[i].fd);
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