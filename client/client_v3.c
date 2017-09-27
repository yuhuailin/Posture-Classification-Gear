//Written by Yuhuai Lin, Mar. 16, 2016
//In this version，the client builds a socket and send its id number and the results of 9DOF to the server 9DOF
//May 3.yuhuai. the message sending to server has different format. Each attributes are seperated by a "*".
//may 11. yuhuai: use 8 attributes

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "client_v3.h"

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int client(struct PackageToServer *param)
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    char buffer[256];
    portno = param->portNum;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(param->hostAddress);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        return 1; //error
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    bzero(buffer,256);
    snprintf(buffer,255,"%d%f*%f*%f*%f*%f*%f*%f*%f*",param->client_id,param->attributes_client[0],
    	param->attributes_client[1],param->attributes_client[2],param->attributes_client[3],
    	param->attributes_client[4],param->attributes_client[5],param->attributes_client[6],
    	param->attributes_client[7]);
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0)
        error("ERROR writing to socket");
    close(sockfd);
    return 0; //success
}
