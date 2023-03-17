#define MSG_LEN 1024
#define SERV_PORT "8080"
#define SERV_ADDR "127.0.0.1"
#define NBR_CO 10
#define NICK_LEN 128

struct client {
	int client_id;
	int sfd;
	uint32_t ADDR;
	u_short PORT;
	char nick[NICK_LEN];
	struct client* next;
};

struct start_client {
	struct client* first;
};