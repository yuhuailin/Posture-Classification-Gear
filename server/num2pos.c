//A simple server in the internet domain using TCP
//The port number is passed as an argument
//Written by Yuhuai Lin, May. 7, 2016. translate the classifier output to posture
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <math.h>
#include "floatfann.h"

//input length
#define LENGTH_INPUT 3


//posture parameters for dynamic


char* num2pos(fann_type input[])
{
    char *pos = malloc(sizeof(char)*256);
    if(pos == NULL)
    {
	printf("Memory allocation error, exiting ...\n");
	exit(-1);
    }

    //rounding
    int i;
    for (i = 0;i <= LENGTH_INPUT;i++)
    {
        if(input[i] < 0.5)
            input[i]=0;
        else if(input[i] > 0.5)
            input[i]=1;
    }
    //store in a string
    char num[20];
    sprintf(num, "%d%d%d", (int)input[0], (int)input[1], (int)input[2]);
    //printf("The converted char array is: %c%c%c\n",num[0],num[1],num[2]);
  
    //determine the posture.
    if(!strncmp(num, "000",3))
        pos = "sitting";
    else if(!strncmp(num, "001",3)) 
	pos = "standing";
    else if(!strncmp(num, "010",3)) 
	pos = "leaning back";
    else if(!strncmp(num, "011",3)) 
	pos = "napping on desk";
    else if(!strncmp(num, "100",3)) 
        pos = "lie down";
    else if(!strncmp(num, "101",3)) 
	pos = "walking";
    else
        pos = "undefined";
    return pos;
}

