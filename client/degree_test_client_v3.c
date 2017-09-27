//March 15. Yuhuai: client with iir.
//May 4th. Yuhuai: exclude iir, use 10 attributes to determine motion.
//May 11. yuhuai: use 8 attributes
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>                                                          
#include <mraa/i2c.h>
#include <mraa/gpio.h>                                                        
#include <math.h>                                                            
#include "LSM9DS0.h"                                                         
#include <pthread.h>
#include "client_v3.h"


#define CLIENTID 1
#define PI 3.1415926
#define PACK_TIME 3
#define SAMPLE_RATE 25
#define PACK_SIZE 75
#define NUM_ATTRIBUTES 8


//a flag for the while loop
sig_atomic_t volatile run_flag = 1;

data_t accel_data;   
float a_res;         
mraa_i2c_context accel;
accel_scale_t a_scale = A_SCALE_4G;
float az_default = -1;

//variables
int count = 0;
float ax,ay,az;
float ax_min = 2;
float ay_min = 2;
float az_min = 2;
float ax_max = -2;
float ay_max = -2;
float az_max = -2; 
float ax_sum = 0;
float ay_sum = 0;
float az_sum = 0;
float length[PACK_SIZE];
float length_sum = 0;
float length_avg = 0;   
float length_dif_sq_sum = 0; 
float length_dif_sum = 0; 

//create a new thread
pthread_t thread0;

//signal handler for Ctrl-C
void do_when_interrupted(int sig)
{
	//if an interrupted signal invoked, set the flag to false
	if (sig == SIGINT)
		run_flag = 0;
}

//To set the reading rate same to sampling rate,we write a time handler
void timer_handler(int signum)
{
	if(count != PACK_SIZE)
	{
	//Read the sensor data and calculate the rotary angle
        accel_data = read_accel(accel, a_res);
       
        ax = accel_data.x; 
        ay = accel_data.y; 
        az = accel_data.z;
        ax_sum += ax; 
        ay_sum += ay; 
        az_sum += az;
        if(ax>ax_max)
            ax_max = ax;
        if(ax<ax_min)
            ax_min = ax;
        if(ay>ay_max)
            ay_max = ay;
        if(ay<ay_min)
            ay_min = ay;
        if(az>az_max)
            az_max = az;
        if(az<az_min)
            az_min = az;
        length[count] = sqrt(ax*ax + ay*ay + az*az);
        length_sum += length[count];
         
	   count++;
	}
}
//create an entry point for the thread
void *my_entry_function(void *param)
{
	client((struct PackageToServer*)param);
}

int main(int argc, char *argv[])
{
	//initialize sensors, set scale, and calculate resolution.
	accel = accel_init();                                                
    set_accel_scale(accel, a_scale);                                     
    set_accel_ODR(accel,A_ODR_25);                                       
    a_res = calc_accel_res(a_scale);                                     
  
    //set up parameters for time handler
	struct itimerval timer={0};
	timer.it_value.tv_usec = 40000;
	timer.it_interval.tv_usec = 40000;
	signal(SIGALRM, &timer_handler);
	setitimer(ITIMER_REAL, &timer, NULL);

    //create a package struct to store info to send
    struct PackageToServer *param;
    param = malloc(sizeof(struct PackageToServer));
    param->hostAddress = malloc(16 * sizeof(char));
    strcpy(param->hostAddress, argv[1]);
    param->portNum = atoi(argv[2]);
    param->client_id = CLIENTID;

	signal(SIGINT, do_when_interrupted);


	while(run_flag)
	{
		if(count == PACK_SIZE)                                            
            {                                                            
            //calculate all the extracted parameters  
            length_avg = length_sum/PACK_SIZE;                                                       
            param->attributes_client[0] = ax_sum/PACK_SIZE; //0. ax_avg
            param->attributes_client[1] = ay_sum/PACK_SIZE; //1. ay_avg
            param->attributes_client[2] = az_sum/PACK_SIZE; //2. az_avg
            param->attributes_client[3] = ax_max - ax_min; //3. ax_range
            param->attributes_client[4] = ay_max - ay_min; //4. ay_range
            param->attributes_client[5] = az_max - az_min; //5. az_range                  

            int i;
	    for(i = 0; i<PACK_SIZE; i++)
            {
                length_dif_sq_sum += (length[i]-length_avg)*(length[i]-length_avg);
            }
            for (i = 0; i<PACK_SIZE-1; i++)
                length_dif_sum += fabs(length[i+1]-length[i]);

            param->attributes_client[6] = sqrt(length_dif_sq_sum/PACK_SIZE); //8. std
            param->attributes_client[7] = length_dif_sum/PACK_TIME; //9. AVC
		
		printf("8 attributes are:\nax_avg: %f\nay_avg: %f\naz_avg: %f\nax_range: %f\nay_range: %f\naz_range: %f\nstd: %f\navc: %f\n",param->attributes_client[0],
param->attributes_client[1],param->attributes_client[2],param->attributes_client[3],
param->attributes_client[4],param->attributes_client[5],param->attributes_client[6],
param->attributes_client[7]);
		
			//send the sensed tilting angel to IoT cloud
			//char cmd[50];
			//snprintf (cmd,sizeof(cmd), "./send_udp.js Zangle %f",phi_avg);
			//system(cmd);
            
            //Set all counters to zero after one sensing cycle
            ax_max = -2;
	    ay_max = -2;
	    az_max = -2;
	    ax_min = 2;
	    ay_min = 2;
  	    az_min = 2;
            ax_sum = 0;
	    ay_sum = 0;
	    az_sum = 0; 
            length_sum = 0;
	    length_avg = 0;
	    length_dif_sq_sum = 0;
	    length_dif_sum = 0;                                                                                
            count = 0; 

            //send the data to another edison
            //call the newly created thread
			if(pthread_create(&thread0,NULL,my_entry_function,param))
			{
				fprintf(stderr, "Error creating thread\n");
				return 1;
			}
            //wait until the new thread finished.
	    pthread_join(thread0,NULL);
            }
	} 
	return 0;
}
