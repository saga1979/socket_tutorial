
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define MAX_CLINETS 1000

static int clientfds[MAX_CLINETS] = {0};

static fd_set readfds;
static fd_set writefds;
static struct timeval timeout;
static int sockfd = -1;
static char buffer[256] = {0};
     

void error(char *msg)
{
    perror(msg);
    exit(1);
}

void process_request()
{	
	
	for(int i=0; i<MAX_CLINETS; i++)
	{
		if(clientfds[i] <= 0)
			continue;
		if(FD_ISSET(clientfds[i], &readfds))
		{
			bzero(buffer,256);			
		    int n = read(clientfds[i],buffer,255);
		    if (n < 0) 
		    {
				perror("ERROR reading from socket");
				close(clientfds[i]);
				clientfds[i] = -1;//从列表中清除
				continue;
			}
			time_t cur = time(0);
			struct tm * ptm = localtime(&cur);
			printf("%s:", asctime(ptm));
			
		    printf("msg: %s\n",buffer);	
		    if(FD_ISSET(clientfds[i], &writefds))
			{
				int n = write(clientfds[i],"I got your message",18);
			    if (n < 0) error("ERROR writing to socket");		
			}	   	
		}
		
	}
	if(FD_ISSET(sockfd, &readfds))
	{
	 
		 struct sockaddr_in cli_addr;
		 int clilen = sizeof(cli_addr);
	     int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	     if (newsockfd < 0) 
	          error("ERROR on accept");
	     for(int i=0; i< MAX_CLINETS; i++)
	     {
			 if(clientfds[i] <=0 )
			 {
				 clientfds[i] = newsockfd;
				 break;
			 }
			 
		 }	     
	}
	
}

static const char* SLEEP_MSG = "sleep 1 second....\n";

int main(int argc, char *argv[])
{
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     
     struct sockaddr_in serv_addr;
     
     sockfd = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     int portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
		error("ERROR on binding");
     listen(sockfd,5);    
     
    
     while(1)
     {
		 FD_ZERO(&readfds);
		 FD_ZERO(&writefds);
		 int max_fd = sockfd;
		 FD_SET(sockfd, &readfds);
		 for(int i =0 ;i < MAX_CLINETS; i++)
		 {
			 if(clientfds[i] > 0)
			 {
				 FD_SET(clientfds[i], &readfds);
				 FD_SET(clientfds[i], &writefds);
			 }
			 max_fd = max_fd > clientfds[i]?max_fd:clientfds[i];
		 }
		 timeout.tv_sec = 0;
		 timeout.tv_usec = 100000;//0.1sec
		 
		 int ret = select(max_fd+1, &readfds, &writefds, 0, &timeout);
		 if(ret == -1)
		 {
			 error("select:");
		 }
		 if(ret == 0)
		 {
			 //如果没有任何可操作文件描述符，等待指定的时间，如1秒
			 timeout.tv_sec = 1;
			 timeout.tv_usec = 0;
			 //fprintf(stderr, "");
			 write(STDOUT_FILENO, SLEEP_MSG, strlen(SLEEP_MSG));
			 select(0,0,0,0,&timeout);
			 continue;
		 }
		 process_request();
		 
	 }  
     
    
     return 0; 
}
