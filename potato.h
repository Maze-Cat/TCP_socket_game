#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

/* Constraint on the number of hops */
#define MAX_HOPS 512 
#define MAX_PORT 65535


typedef struct player_t{
	int num;
	int total_num;
	int socket_id;
	int left_num;
	int left_skt;
	int right_num;
	int right_skt;
	int is_connect;

	struct sockaddr_in ad;
	struct sockaddr_in left_ply;
	struct sockaddr_in right_ply;
        
	
	
}player;

typedef struct potato_t{
	int num_hop;
	int i;
	int trace[MAX_HOPS];
     int end;
      int valid;
  int next_ply;
}potato;

