  #include <iostream>
  #include <cstring>
  #include <sys/socket.h>
  #include <netdb.h>
  #include <unistd.h>
  #include "potato.h"
  #include <arpa/inet.h>
using namespace std;

int main(int argc, char *argv[])
{
    //take in inputs
  if(argc != 4){
    perror("usage:./ringmaster <port_num> <num_players> <num_hops>\n");
    exit(EXIT_FAILURE);
  }

  if(atoi(argv[1])> MAX_PORT || atoi(argv[1])<0){
    perror("invalid port number!\n");
    exit(EXIT_FAILURE);
  }

  if(atoi(argv[2]) <= 1){
    perror("invalid player number!\n");
    exit(EXIT_FAILURE);
  }

  if(atoi(argv[3])> MAX_HOPS || atoi(argv[3])<0){
    perror("invalid hops!\n");
    exit(EXIT_FAILURE);
  }

  char*  port_num;
  static unsigned long num_players;
  static unsigned long num_hops;

  port_num = argv[1];
  num_players = atol(argv[2]);
  num_hops = atol(argv[3]);

  printf("Potato Ringmaster\n");
  printf("Players = %lu\n", num_players);
  printf("Hops = %lu\n", num_hops);

  potato hot_potato;
  memset(&hot_potato,0,sizeof(hot_potato));
  hot_potato.num_hop = num_hops;


    //adapted from TCP example from class
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;



  memset(&host_info, 0, sizeof(host_info));

  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags    = AI_PASSIVE;

  char host[128];
  gethostname(host, sizeof(host));

  status = getaddrinfo(host, port_num, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    return -1;
    } //if

    socket_fd = socket(host_info_list->ai_family, 
     host_info_list->ai_socktype, 
     host_info_list->ai_protocol);
    if (socket_fd == -1) {
      cerr << "Error: cannot create socket" << endl;
      return -1;
    } //if

    int yes = 1;
    status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
      cerr << "Error: cannot bind socket" << endl;
      return -1;
    } //if

    //cout<< "listening ports"<<endl;
    status = listen(socket_fd, 100);
    if (status == -1) {
      cerr << "Error: cannot listen on socket" << endl; 
      return -1;
    } //if

    //set up players
    player players[num_players];
    for(int i=0; i<num_players;i++){
      players[i].num = i;
      players[i].is_connect = 0;
      players[i].total_num = num_players;

      
      socklen_t socket_addr_len = sizeof(struct sockaddr);
      
      players[i].socket_id = accept(socket_fd, (struct sockaddr*)&players[i].ad, &socket_addr_len);
      
      //<< "socketid of "<<i<< " is"<< players[i].socket_id<<endl;
      if (players[i].socket_id == -1) {
        cerr << "Error: cannot accept connection on socket" << endl;
        return -1;
      } 
      players[i].is_connect = 1;

      send(players[i].socket_id, &players[i],sizeof(players[i]),0);

      status = recv(players[i].socket_id, &players[i],sizeof(players[i]),0);
      if(status<0){
        cerr<<"can't recv player id"<<endl;
        exit(EXIT_FAILURE);
      }
    }

    //set up circle

    for(int i=0;i<num_players;i++){
      if(players[i].num == 0){
        players[i].left_num = num_players - 1;
        players[i].left_skt = players[num_players-1].socket_id;
        players[i].right_num = players[i].num + 1;
        players[i].right_skt = players[players[i].right_num].socket_id;
      }else if(players[i].num == num_players-1){
        players[i].left_num = players[i].num - 1;
        players[i].left_skt = players[players[i].left_num].socket_id;
        players[i].right_num = 0;
        players[i].right_skt = players[0].socket_id;
      }else{
        players[i].right_num = players[i].num + 1;
        players[i].right_skt = players[players[i].right_num].socket_id;
        players[i].left_num = players[i].num - 1;
        players[i].left_skt = players[players[i].left_num].socket_id;
      }
      players[i].right_ply = players[players[i].right_num].ad;
      players[i].left_ply = players[players[i].left_num].ad;

    }

    for(int i=0;i<num_players;i++){
      send(players[i].socket_id, &players[i], sizeof(players[i]), 0);
    }
    for(int i = 0;i<num_players;i++){
      char bf[8];
      int ack =recv(players[i].socket_id, &bf, sizeof(bf), 0);
      if(ack<0){
        perror(" ACK fails!\n");
        exit(EXIT_FAILURE);
      }

      printf("Player %d is ready to play\n", players[i].num);

    }

    //monitor file descriptors
    fd_set ringmaster; // master file descriptor 
    fd_set read_fds; // for select() 
    int fdmax; //max file descriptor num
    fdmax = players[num_players-1].socket_id;

    FD_ZERO(&ringmaster); // clear
    FD_ZERO(&read_fds);

    srand((unsigned int)time(NULL) );
    int start = rand()% num_players;
    
    for (int i = 0; i < num_players; i++){
      FD_SET(players[i].socket_id, &ringmaster);
    }
    

    //if hop=0, shut down the game
    if(num_hops == 0){
      for(int j=0;j<num_players;j++){
          potato ptt;
          memset(&ptt,0,sizeof(ptt));
          ptt.end = 1;
          ptt.valid =1;
          send(players[j].socket_id,&ptt,sizeof(ptt),0);
          //cout<<"close"<<endl;
          close(players[j].socket_id);
        }
      freeaddrinfo(host_info_list);  
      close(socket_fd);
      //cout<<"end master"<<endl;
      return 0;
    }

    //start game throw potatoes
    printf("Ready to start the game, sending potato to player %d\n", start);
    hot_potato.valid =1;
    status = send(players[start].socket_id, &hot_potato, sizeof(hot_potato), 0);
    if(status<0){
      cout<<"cannot send"<<endl;
    }
    
    

    
    while(1){
    read_fds = ringmaster; //in case overwrite master fdset
    if(select(65535,&read_fds,NULL,NULL,NULL)== -1){
      printf("select()");
      exit(EXIT_FAILURE);
    }

    for(int i =0; i<num_players;i++){
      if(FD_ISSET(players[i].socket_id, &read_fds)){ //end game
        int rv=0;
        
        rv = recv(players[i].socket_id,&hot_potato,sizeof(hot_potato),0);
        if(rv<0){
         printf("end game recv()\n");
         exit(EXIT_FAILURE);
        }
        //cout<<"recv end potato"<<endl;

       if(hot_potato.valid==1){
        
        for(int j=0;j<num_players;j++){
          potato ptt;
          memset(&ptt,0,sizeof(ptt));
          ptt.end = 1;
          ptt.valid =1;
          send(players[j].socket_id,&ptt,sizeof(ptt),0);
          //cout<<"close"<<endl;
          close(players[j].socket_id);
        }
      }
    }
  }

   printf("Trace of potato:\n");
   for(int i =0;i<num_hops;i++){
    if(i == num_hops-1){
     printf("<%d>\n",hot_potato.trace[i]);
   }else{
     printf("<%d>,",hot_potato.trace[i] );
   }
 }


 freeaddrinfo(host_info_list);  
 close(socket_fd);
 return 0;

    }//end while

  

  return 0;
  }//end main
