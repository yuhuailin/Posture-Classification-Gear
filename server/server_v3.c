//The server analyzes the data received from each client
//Written by Yun Zhang, Mar. 14, 2016
//12 inputs for each 9DOF
//May.1st: yuhuai info_decription new translates 10 attributes out of message received and set the status of client message as receied.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include "server_v3.h"

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void info_decryption(char message[], struct Result *param)
{
    int length = (int)strlen(message);
    int device_id = message[0]- '0';
    int sequence_count = 0;
    int i,j;
    for(i=1;i<length;i++)
    {
        int word_count = 0;
        for(j=i;j<length;j++)
        {
            if(message[j]!='*')
                word_count++;
            else
            {
                char temp[word_count];
                strncpy(temp,message+i,word_count);
                param->attributes[sequence_count][device_id]=atof(temp);
                sequence_count++;
                i = i + word_count;
                j = length;
            }
        }
    }
    param->received[device_id]=true;
}

int server(struct Result *param)
{
    int sockfd, newsockfd;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sockfd < 0) 
        error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(param->portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR on binding");
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    
    while(1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) 
            error("ERROR on accept");
        bzero(buffer,256);
        n = read(newsockfd,buffer,255);

        if (n < 0) 
            error("ERROR reading from socket");
       // printf("Here is the message: %s\n",buffer);
        info_decryption(buffer, param);
        close(newsockfd);
     }

    close(sockfd);
    return 0; 
}

