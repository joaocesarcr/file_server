#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#include "./h/message_struct.h"
#include "./commands.c"
#define PORT 4000

int createConnection(char *argv[]);

int main(int argc, char *argv[])
{
    if (strcmp(argv[1], "command\n")) {
      printf("Testando inputs\n");
      handleInput();
    }

		if (argc < 2) {
      fprintf(stderr,"usage %s hostname\n", argv[0]);
      exit(0);
    }

		int sockfd = createConnection(argv);
		char buffer[MAX_MESSAGE_LENGTH];

    MESSAGE a;	
    a.number = 0;;	
    int running = 1;
    do {
      int n;
      strncpy(a.data, "Sending packet", MAX_MESSAGE_LENGTH);
      //printf("teste: %s\n", a.data);

      /* write in the socket */
      n = write(sockfd, (void*) &a, sizeof(MESSAGE));
      if (n < 0) 
        printf("ERROR writing to socket\n");

      bzero(buffer,256);
    
      /* read from the socket */
      do {
        n = read(sockfd, buffer, MAX_MESSAGE_LENGTH);
      } while (n < MAX_MESSAGE_LENGTH);

      if (n < 0) 
        printf("ERROR reading from socket\n");

      printf("%d - Answer: %s\n",a.number,buffer);
      sleep(4);
      a.number++;
      if (a.number == 3)
        running = 0;

    } while (running);
    printf("Ending connection\n");

    close(sockfd);
    return 0;
}
int createConnection(char *argv[]) {
		int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
	

    server = gethostbyname(argv[1]);
    if (server == NULL) {
      fprintf(stderr,"ERROR, no such host\n");
      exit(0);
    }
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1) 
			printf("ERROR opening socket\n");
    
    serv_addr.sin_family = AF_INET;     
    serv_addr.sin_port = htons(PORT);    
    serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
    bzero(&(serv_addr.sin_zero), 8);     
    
    
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
      printf("ERROR connecting\n");
		return sockfd;

}
