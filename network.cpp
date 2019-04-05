#include <stdio.h> 
#include "network.h"
#include "arpa/inet.h"
#include "sys/socket.h"
#include <cstring>
#include <stdlib.h>

#define PORT 5805
#define IP "10.34.76.2"
//#define IP "10.34.76.75"
//#define IP "127.0.0.1"

int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
sockaddr_in server;

void setupUDP() {

	const char *buffer = "test UDP packet";
	int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
	//bzero(&server, sizeof(server));	
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(IP);
	server.sin_port = htons(PORT);
}

int sendUDP(std::vector<exp_data> d) {
	float *data = (float *)malloc(sizeof(float)*4*d.size());
	int c = 0;
	for(int i = 0; i < d.size(); i++) {
		data[c] = (float)d[i].centroid.x;
		data[c+1] = (float)d[i].centroid.y;
		data[c+2] = (float)d[i].connectorMag;
		data[c+3] = (float)d[i].distance;
		c+=4;
	}

	//printf("%f", data[0]);
	

	sendto(s, data, sizeof(float)*4*d.size(), 0, (sockaddr*)&server, sizeof(server));
	free(data);	
	return 0; 
}
