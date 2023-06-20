#include "./SerDes/message_struct.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 4000

void client_thread(int newsockfd, int sockfd);
int main(int argc, char *argv[]) {
	int sockfd, newsockfd, n, bindReturn;
	socklen_t clilen;
	char buffer[MAX_MESSAGE_LENGTH];
	struct sockaddr_in serv_addr, cli_addr;
	clilen = sizeof(struct sockaddr_in);
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) 
		printf("ERROR opening socket");
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);     
    
  //  assigns the address specified by addr to the socket referred to
	// by the file descriptor sockfd
  bindReturn = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if (bindReturn < 0) 
		printf("ERROR on binding");
	
	// tells the socket that new connections shall be accepted
	listen(sockfd, 5);
	
	// get a new socket with a new incoming connection
	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	if (newsockfd == -1) 
		printf("ERROR on accept");
	
  else {
    client_thread(newsockfd, sockfd);
		
  }
	return 0; 
}

void client_thread(int newsockfd, int sockfd){
  MESSAGE a;

  int running = 1, n;
  while (running) {
    /* read from the socket */
    n = read(newsockfd,(void*) &a, sizeof(a));

    if (n < 0) 
      printf("ERROR reading from socket\n");
    else { 
      printf("%s: %d\n",a.data, a.number);
    }
    
    /* write in the socket */ 
    char message[256];
    snprintf(message, sizeof(message), "I got your message: %s", a.data);
    //printf("teste: %s", message);
    n = write(newsockfd,message, MAX_MESSAGE_LENGTH);
    if (n < 0) 
      printf("ERROR writing to socket\n");

    if (a.number == 10) {
      close(newsockfd);
      close(sockfd);
      running = 0;
    }

  }

}
