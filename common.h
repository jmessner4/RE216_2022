#define MSG_LEN 1024
/* #define SERV_PORT "8080"
#define SERV_ADDR "127.0.0.1" */

struct info{
    short s;
    long l;
};

struct client {
    int client_id;
	int sfd;
	uint32_t ADDR;
	u_short PORT;
    char* nick;
	struct client* next;
};

struct start_client {
	struct client* first;
};