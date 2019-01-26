#include <stdio.h> 
#include "network.h"
#include "arpa/inet.h"
#include "sys/socket.h"
#include <cstring>

#define PORT 8000
#define IP "10.34.76.75"

int sendUDP() {
	//printf("test \n");
	sockaddr_in server;
	//char buffer[256];
	const char *buffer = "test UDP packet";
	int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
	//bzero(&server, sizeof(server));	
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(IP);
	server.sin_port = htons(PORT);
	sendto(s, buffer, strlen(buffer)+1, 0, (sockaddr*)&server, sizeof(server));
	
	return 0; 
}
