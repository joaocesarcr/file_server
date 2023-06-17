#include "./SerDes/message_struct.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 4000

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, n;
	socklen_t clilen;
	char buffer[MAX_MESSAGE_LENGTH];
	struct sockaddr_in serv_addr, cli_addr;
	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
        printf("ERROR opening socket");
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);     
    
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
		printf("ERROR on binding");
	
	listen(sockfd, 5);
	
	clilen = sizeof(struct sockaddr_in);
	if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1) 
		printf("ERROR on accept");
	
	bzero(buffer, 256);
	MESSAGE a;
	
//  while (1) {
    /* read from the socket */
    n = read(newsockfd, a.data, MAX_MESSAGE_LENGTH);

    if (n < 0) 
      printf("ERROR reading from socket\n");
    else
      printf("Here is the message: %s\n", a.data);
    
    /* write in the socket */ 
		char message[256];
		snprintf(message, sizeof(message), "I got your message: %s", a.data);
		//printf("teste: %s", message);
    n = write(newsockfd,message, MAX_MESSAGE_LENGTH);
    if (n < 0) 
      printf("ERROR writing to socket\n");

//  }
  close(newsockfd);
  close(sockfd);
	return 0; 
}
