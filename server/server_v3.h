//change to use bools to determine if both signals are received.
//May.3: yuhuai. Use 8 attibutes for each client and server and put them in a N-D(2D in our case) array. 


//Attributes sequence:
//0. ax_avg
//1. ay_avg
//2. az_avg
//3. ax_range
//4. ay_range
//5. az_range
//6. std
//7. avc


#ifndef server_h
#define server_h
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define NUM_9DOF 2
#define PACK_SIZE 75
#define NUM_ATTRIBUTES 8


struct Result
{
	int portno;
	float attributes[NUM_ATTRIBUTES][NUM_9DOF];
	bool received[NUM_9DOF];
};

int server(struct Result *param);
void info_decryption(char message[], struct Result *param);

#endif /* server_h */