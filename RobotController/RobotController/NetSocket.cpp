#include "NetSocket.h"

#include <cstdlib>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <stdio.h>
#include <cerrno>

NetSocket::NetSocket(char* arg_port, char* arg_ip_address)
{
	size_id = sizeof(id);
	size_range = sizeof(range);	
	memset(&their_addr, 0, sizeof(their_addr));
	strcpy(port, arg_port);
	strcpy(ip_address, arg_ip_address);
}

NetSocket::~NetSocket()
{
	freeaddrinfo(servinfo);
	WSACleanup();
}

int NetSocket::openSocket()
{
	// Taken from "Beej's Networking Guide"
	int optval;
	struct addrinfo hints, *p;
	int rv;
	char s[INET6_ADDRSTRLEN], init_msg[8];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	optval = 1;

	if ((WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
		printf("ERROR: WSAStartup failed.");
		return -1;
	}

	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		printf("ERROR: getaddrinfo: %s\n", gai_strerror(rv));
		freeaddrinfo(servinfo);
		return -1;
	}

	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == SOCKET_ERROR) {
			printf("ERROR: Socket creation error. WSA Error: %d\n", WSAGetLastError());
			continue;
		}
		if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof optval) == -1) {
			printf("ERROR: Setsockopt error. WSA Error: %d\n", WSAGetLastError());
			continue;
		}
		if (bind(socket_fd, p->ai_addr, p->ai_addrlen) != 0) {
			closesocket(socket_fd);
			printf("ERROR: Bind error. WSA Error: %d\n", WSAGetLastError());
			continue;
		}
		break;
	}

	if (p == NULL) {
		printf("Failed to connect\n");
		freeaddrinfo(servinfo);
		return -1;
	}
/*	else {
		inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), s, sizeof s);
		printf("Bound socket to address: %s\n\n", s);
	}*/

	// Wait to receive initial message and store server address info. BLOCKING
	printf("Waiting on handshake...\n");
	addr_len = sizeof(their_addr);
	rv = recvfrom(socket_fd, init_msg, sizeof(init_msg), NULL, (struct sockaddr *)&their_addr, &addr_len);

	if (rv == SOCKET_ERROR) {
		printf("ERROR: recvfrom error. WSAError: %s/n", WSAGetLastError());
		freeaddrinfo(servinfo);
		return -1;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)&their_addr), s, sizeof s);
	printf("UDP handshake received from address: %s\nrecvfrom return: %d\nWSAError: %d\n\n", s, rv, WSAGetLastError());

	return 0;
}

// Networking Helper functions copied from "Beej's Networking Guide"
// get sockaddr, IPv4 or IPv6:
void* NetSocket::get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}