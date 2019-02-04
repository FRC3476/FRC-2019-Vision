#include <stdio.h> 
#include "network.h"
#include "arpa/inet.h"
#include "sys/socket.h"
#include <cstring>
#include <stdlib.h>

#define PORT 8000
//#define IP "10.34.76.75"
#define IP "127.0.0.1"

int sendUDP(std::vector<exp_data> d) {
	float *data = (float *)malloc(sizeof(float)*3*d.size());
	int c = 0;
	for(int i = 0; i < d.size(); i++) {
		data[c] = (float)d[i].centroid.x;
		data[c+1] = (float)d[i].centroid.y;
		data[c+2] = (float)d[i].connectorMag;
		c+=3;
	}

	//printf("%f", data[0]);
	sockaddr_in server;
	const char *buffer = "test UDP packet";
	int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
	//bzero(&server, sizeof(server));	
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(IP);
	server.sin_port = htons(PORT);

	sendto(s, data, sizeof(float)*3*d.size(), 0, (sockaddr*)&server, sizeof(server));
	
	return 0; 
}
