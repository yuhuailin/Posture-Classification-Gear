//This version collects and extracts raw acceleration data from server and client 9DOFs in a 3 sec sliding
//window (30 samples): range (3 attributes), mean length (3 attributes), mean acceleration (3 attributes), 
//RMS of length(1 attributes), STD of length (1 attributes) and AVC (1 attributes). These data are then sent 
//into the classifier for posture identification
//This version uses a pre-trained neural network to classify the posture
//May. 11, 2016: delete length_avg and rms attributes
//Tested on May. 18, 2016: okay to present on the final

#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>  
#include <stdbool.h>                                                        
#include <mraa/i2c.h>
#include <mraa/gpio.h>                                                        
#include <math.h> 
#include <pthread.h>
#include <string.h>
#include "LSM9DS0.h"
#include "floatfann.h"                                                       
#include "server_v3.h" 

#define SERVERID 0
#define CLIENTID 1
#define PI 3.1415926
#define PACK_TIME  2
#define SAMPLE_RATE  25 //Hz
#define PACK_SIZE 75
#define NUM_9DOF 2
#define NUM_ATTRIBUTES 8


/************************************************ Variables ************************************************/
//a flag for the while loop
sig_atomic_t volatile run_flag = 1;

//variables for 9DOF sensors
data_t accel_data;   
float a_res;         
mraa_i2c_context accel;
accel_scale_t a_scale = A_SCALE_4G;   

//variables for raw data collection and extraction                                                           
int count = 0;
float ax,ay,az;
float ax_max = -2;
float ay_max = -2;
float az_max = -2;
float ax_min = 2;
float ay_min = 2;
float az_min = 2;
float ax_sum = 0;
float ay_sum = 0;
float az_sum = 0;
float length[PACK_SIZE];
float length_sum = 0;
float length_avg = 0;
float length_dif_sq_sum = 0;
float length_dif_sum = 0;

//variables for creating threads
pthread_t thread0;

FILE* fp;


/************************************************ Functions ************************************************/
//signal handler for Ctrl-C
void do_when_interrupted(int sig)
{
	//if an interrupted signal invoked, set the flag to false
	if (sig == SIGINT)
		run_flag = 0;
}

//To set the reading rate same to sampling rate, we write a time handler
void timer_handler(int signum)
{
	if(count != PACK_SIZE)
	{
        //Read the sensor data and calculate the orientation angles and length
        accel_data = read_accel(accel, a_res);
        ax = accel_data.x;
        ay = accel_data.y;
        az = accel_data.z;
        ax_sum += ax;
        ay_sum += ay;
        az_sum += az;
        ax_max = (ax>ax_max)? ax : ax_max;
        ax_min = (ax<ax_min)? ax : ax_min;
        ay_max = (ay>ay_max)? ay : ay_max;
        ay_min = (ay<ay_min)? ay : ay_min;
        az_max = (az>az_max)? az : az_max;
        az_min = (az<az_min)? az : az_min;
        
    	length[count] = sqrt(ax*ax + ay*ay + az*az);
    	length_sum += length[count];

		count++;
	}
}

//create thread function
void* my_entry_function(void* param)
{
	server((struct Result *) param); 	
}


int main(int argc, char *argv[])
{
	fp = malloc(sizeof(FILE*));
	fp = fopen("pos_attributes_output.txt","a");
	//fp = fopen("pos_attributes_output.txt","w");
	if (fp == NULL)
	{
		printf("Error openining file!\n");
		exit(1);
	}

	//initialize receiving data from clients
	struct Result *param;
	param = malloc(sizeof(struct Result));
	int i;
	for (i = 0; i < NUM_9DOF; i++)
		param->received[i] = false;
	param->portno = atoi(argv[1]); 

	if(pthread_create(&thread0,NULL,my_entry_function,param))
	{                       
		fprintf(stderr, "Error creating thread\n");   
		return 1;         
    }  

	//initialize sensors, set scale, and calculate resolution
	accel = accel_init();                                                
    set_accel_scale(accel, a_scale);                                     
    set_accel_ODR(accel,A_ODR_25);                                       
    a_res = calc_accel_res(a_scale);
    
    //initialize the fann
    struct fann* ann = fann_create_from_file("pos_train.net");
    fann_type *fann_out;
    fann_type fann_input[2*NUM_ATTRIBUTES];

 	//set up parameters for the time handler
	struct itimerval timer={0};
	timer.it_value.tv_usec = 40000;
	timer.it_interval.tv_usec = 40000;
	signal(SIGALRM, &timer_handler);
	setitimer(ITIMER_REAL, &timer, NULL);

	signal(SIGINT, do_when_interrupted);
    

	while(run_flag)
	{
		if(count == PACK_SIZE)                                            
        {   
        	//calculate all the extracted parameters  
        	length_avg = length_sum/PACK_SIZE;
            param->attributes[0][SERVERID] = ax_sum/PACK_SIZE; //0. ax_avg
            param->attributes[1][SERVERID] = ay_sum/PACK_SIZE; //1. ay_avg
            param->attributes[2][SERVERID] = az_sum/PACK_SIZE; //2. az_avg
            param->attributes[3][SERVERID] = ax_max - ax_min; //3. ax_range
            param->attributes[4][SERVERID] = ay_max - ay_min; //4. ay_range
            param->attributes[5][SERVERID] = az_max - az_min; //5. az_range

            for(i = 0; i<PACK_SIZE; i++)
            	length_dif_sq_sum += (length[i]-length_avg)*(length[i]-length_avg);
        
            for (i = 0; i<PACK_SIZE-1; i++)
            	length_dif_sum += fabs(length[i+1]-length[i]);

            param->attributes[6][SERVERID] = sqrt(length_dif_sq_sum/PACK_SIZE); //6. std
            param->attributes[7][SERVERID] = length_dif_sum/PACK_TIME; //7. AVC

            //Set all counters to zero after one sensing cycle
            ax_max = ay_max = az_max = -2;
            ax_min = ay_min = az_min = 2;
			ax_sum = ay_sum = az_sum = 0; 
			length_sum = length_avg = 0;
			length_dif_sq_sum = length_dif_sum = 0;
			count = 0;	
			param->received[SERVERID] = true;
            
            //Check whether the server has got the data from itself and all the clients
            bool received_all_data = true;
            for(i = 0; i<NUM_9DOF; i++)
                if(param->received[i] == false)
                {
                    received_all_data = false;
                    break;
                }
            
             if(received_all_data == true)
             {
                 for (i = 0; i < NUM_9DOF; i++)
                     param->received[i] = false;
                 
                //prepare the input for the classifer
                 for(i = 0; i < NUM_ATTRIBUTES; i++)
                 {
                     fann_input[i] = param->attributes[i][SERVERID];
                     fann_input[i+NUM_ATTRIBUTES] = param->attributes[i][CLIENTID];
                 }
                 
                 //calculate the output of the fann classifier
                 fann_out = fann_run(ann,fann_input);
                 
                // printf("The output of fann is: %f %f %f\n",fann_out[0],fann_out[1],fann_out[2]);
                 char *pos = num2pos(fann_out);
                 printf("The posture of the user is: %s\n", pos);
                 
                 //Write attributes into txt file
                 //for(i = 0; i < NUM_ATTRIBUTES; i++)
                 //{
                 //   fprintf(fp,"%f ", param->attributes[i][SERVERID]);
                 //   printf("%f\n", param->attributes[i][SERVERID]);
                 //}
                 //for (i = 0; i < NUM_ATTRIBUTES; i++)
                 //{
                 //   fprintf(fp,"%f ", param->attributes[i][CLIENTID]);
                 //   printf("%f\n", param->attributes[i][SERVERID]);
	         //}
                 //    fprintf(fp,"\n1 1 0\n");
             }
        }
        
	}
	return 0;
}

