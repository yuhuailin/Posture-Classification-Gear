//March 15. Yuhuai: include LPF and HPF. Server sends static angle and dynamic status.
//May 3. Yuhuai: exclude LPF and HPF. Use 10 attributes.
//May 11th. Yuhuai: use 8 attributes
#ifndef client_h
#define client_h
#define NUM_ATTRIBUTES 8 

//Attributes sequence:
//0. ax_avg
//1. ay_avg
//2. az_avg
//3. ax_range
//4. ay_range
//5. az_range
//6. std
//7. avc

struct PackageToServer
{
    float attributes_client[NUM_ATTRIBUTES];
    int client_id;
    char* hostAddress;
    int portNum; 
};
int client(struct PackageToServer *param);


#endif /* client_h */
