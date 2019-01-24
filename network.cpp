#include <stdio.h> 
#include "network.h"
#include "arpa/inet.h"
#include "sys/socket.h"

#define PORT 8000

int sendUDP() {
	//printf("test \n");
	struct si;
	char buffer[256];
	int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
	memset((char *), &si, sizeof(si));	
	si.sin_family = AF_INET;
	si.sin_port = htons(PORT);
	si.sin	

	return 0; 
}
