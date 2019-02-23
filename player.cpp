#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "potato.h"

#define LEN 128

using namespace std;


  int main(int argc, char *argv[])
  {
    int status;
    int port_number = atoi(argv[2]);
    int socket_fd, socket_pl;
    int  left,right;
    int fdmax;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;

    const char *hostname = argv[1];
    char buf[LEN];


    player p;
    potato pt;
    memset(&pt,0,sizeof(pt));
    memset(&p,0,sizeof(p));

    if (argc != 3) {
      perror("usage:./player <machine_name> <port_num>\n");
      exit(EXIT_FAILURE);
    }

    if(atoi(argv[2])<0|| atoi(argv[2])>MAX_PORT){
      perror("invalid port number!\n");
      exit(EXIT_FAILURE);
    }

          /* connect to host*/

    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_INET;
    host_info.ai_socktype = SOCK_STREAM;
    status = getaddrinfo(argv[1], argv[2], &host_info, &host_info_list);

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

          status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
          if (status == -1) {
            cerr << "Error: cannot connect to socket" << endl;
            return -1;
          } //if

          status = recv(socket_fd,&p,sizeof(p),0);
          if(status == -1){
            cerr<<"Cannot initial recv"<<endl;
            exit(EXIT_FAILURE);
          }

          /*
            set my socket port（player）
          */

          char host[LEN];
          gethostname(host, sizeof(host));
          struct hostent * player = gethostbyname(host);

          //set left socket
          socket_pl = socket(AF_INET, SOCK_STREAM, 0);
          int yes =1;
          status = setsockopt(socket_pl, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
          for(int i = 5000;i<MAX_PORT;i++){
            int ply_port = i;
            p.ad.sin_family = AF_INET;
            p.ad.sin_port = htons(ply_port);
            memcpy(&p.ad.sin_addr, player->h_addr_list[0], player->h_length);

            if(bind(socket_pl,(struct sockaddr *)&(p.ad), sizeof(p.ad))>=0){
              break;
            }
          }
          if(status<0){
            cout<<"cannot bind()"<<endl;
            exit(-1);
          }
          status = listen(socket_pl,10);
          if(status<0){
            cout <<"cannot listen"<<endl;
            exit(EXIT_FAILURE);
          }

          right = socket(AF_INET, SOCK_STREAM, 0);
          if(right < 0) {
            perror("socket");
            exit(EXIT_FAILURE);
          }

          //send my socket struct sockaddr ad
          send(socket_fd,&p,sizeof(p),0);

          //connect right player
          //receive right info
          status = recv(socket_fd, &p, sizeof(p), 0);
          if (status == -1) {
            cerr << "Error: cannot recv" << endl; 
            return -1;
          } //if
          //cout<<"my socketid "<<p.socket_id<<endl;

         // status = connect(right, (struct sockaddr *)&p.right_ply, sizeof(p.right_ply));
          status = connect(right, (struct sockaddr *)&p.right_ply, sizeof(sockaddr));
          if(status < 0) {
            perror("connect with right player");
            exit(EXIT_FAILURE);
          }
          
          
          socklen_t s = sizeof(p.right_ply);
          //accept left player
          left = accept(socket_pl,(struct sockaddr *)&p.right_ply,&s);
          if(left < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
          }
          send(socket_fd,"connect", 8,0);
          printf("Connected as player %d out of %d total players\n", p.num, p.total_num);

          fd_set gameset,game; 
          FD_ZERO (&game);

          FD_SET(socket_fd,&game);
          FD_SET(left,&game);
          FD_SET(right,&game);

          int dir;
          int rcv_fd;

          //fdmax
          if(socket_fd > right) {
            if(socket_fd > left) {
             fdmax = socket_fd;
           } else {
            fdmax = left;
          }
        } else { 
          if(right > left) {
            fdmax=right;
          }
          fdmax=left;
        }


        while(1){
            //reset fdset
          gameset=game;
            //generste direction
          dir = rand()%2;

          if (select(fdmax + 1, &gameset, NULL, NULL, NULL) == -1){
            perror("select()");
          }

          if (FD_ISSET(socket_fd, &gameset)){
            rcv_fd = socket_fd;
            //printf("Received from ringmaster\n");
          }
          else if (FD_ISSET(left, &gameset)){
            rcv_fd = left;
            //printf("Received from left player %d\n", p.left_num);
          }
          else if (FD_ISSET(right, &gameset)){
            rcv_fd = right;
            //printf("Received from right player %d\n", p.right_num);
          }
          else{
            fprintf(stderr,"Unknown fd error\n");
            exit(EXIT_FAILURE);
          }

          int status = recv(rcv_fd, &pt, sizeof(pt),0);   
          if (status == -1){
            cout<<"recv in game"<<endl;
            exit(-1);
          }

          
          if((pt.end == 1 )&& (pt.valid ==1)){
            break;
          }
          if(pt.valid ==1){
            pt.valid =0;
            
            pt.trace[pt.i]=p.num;
            pt.num_hop--;
            (pt.i)++;
            

            if(pt.num_hop ==0){
              //if(pt.num_hop ==0){
                cout<<"I'm it"<<endl;
              //}
              pt.valid =1;
              int sts= send(socket_fd,&pt,sizeof(pt),0);
              if(sts<0){
                cout<<"cannot send to master"<<endl;
              }
              sts= recv(socket_fd,&pt,sizeof(pt),0);
              if(sts<0){
                cout<<"cannot recv shutdown"<<endl;
              }

              break;
            }else{
              if(dir){
              //send to right
               cout<<"Sending potato to "<<p.right_num<<endl;
               pt.valid =1;
               int flag = send(right,&pt,sizeof(pt),0);
               //cout<<"num_hop:"<<pt.num_hop<<endl;
               if (flag==-1){
                 cout<<"gg"<<endl;
                 exit(EXIT_FAILURE);
               }

             }else{
          //send to left
               cout<<"Sending potato to "<<p.left_num<<endl;
               pt.valid =1;
               int flag = send(left,&pt,sizeof(pt),0);
               if (flag==-1){
                perror("send left error");
                exit(EXIT_FAILURE);
                }
              }//inelse
            }//outelse
          }//valid

      }//while



      freeaddrinfo(host_info_list);
      close(socket_fd);
      close(right);
      close(left);

      return 0;
    }
