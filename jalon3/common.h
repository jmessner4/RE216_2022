#define MSG_LEN 1024
#define SERV_PORT "8080"
#define SERV_ADDR "127.0.0.1"
#define NBR_CO 100
#define NICK_LEN 128
#define USR_CHAT 10
#define CHAT_LEN 10

struct client {
	int client_id;
	int sfd;
	uint32_t ADDR;
	u_short PORT;
	char nick[NICK_LEN];
	struct chatrooms* chat;
	int isinchat;
	struct client* next;
};

struct start_client {
	struct client* first;
};

struct chatrooms {
	int server_id;
	char name[NICK_LEN];
	struct client* users[USR_CHAT];
};