/*****************************************************
*	NetSocket.cpp
*
*	Class for using TCP and UDP sockets.
*
*	Author: Charles Hartsell
*	Date:	3-31-17
*****************************************************/

#include "NetSocket.h"

#include <cstdlib>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <stdio.h>
#include <cerrno>

NetSocket::NetSocket(char* arg_port, char* arg_ip_address, int socktype)
{
	size_id = sizeof(id);
	size_range = sizeof(range);	
	memset(&their_addr, 0, sizeof(their_addr));
	memset(ip_address, 0, sizeof(ip_address));
	strcpy(port, arg_port);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = socktype;
}

NetSocket::NetSocket(char* arg_port, int socktype)
{
	size_id = sizeof(id);
	size_range = sizeof(range);
	memset(&their_addr, 0, sizeof(their_addr));
	memset(ip_address, 0, sizeof(ip_address));
	strcpy(port, arg_port);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = socktype;
	hints.ai_flags = AI_PASSIVE;
}

NetSocket::NetSocket(int socktype)
{
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = socktype;
	hints.ai_flags = AI_PASSIVE;
}

NetSocket::~NetSocket()
{
	freeaddrinfo(servinfo);
	WSACleanup();
}

int NetSocket::openSocket()
{
	if (hints.ai_socktype == SOCK_DGRAM) {
		return (this->openUDPSocket());
	}
	else if (hints.ai_socktype == SOCK_STREAM) {
		return (this->openTCPSocket());
	}
	else {
		printf("ERROR: NetSocket::openSocket unsupported socket type (UDP and TCP supported).\n");
		return -1;
	}
}

int NetSocket::openUDPSocket()
{
	// Taken from "Beej's Networking Guide"
	int optval;
	struct addrinfo *p;
	int rv;

	optval = 1;

	if ((rv = getaddrinfo(ip_address, port, &hints, &servinfo)) != 0) {
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
		printf("Failed to open UDP socket.\n");
		freeaddrinfo(servinfo);
		return -1;
	}
	return 0;
}

int NetSocket::openTCPSocket()
{
	// Taken from "Beej's Networking Guide"
	int optval;
	struct addrinfo *p;
	int rv;

	optval = 1;

	if ((rv = getaddrinfo(ip_address, port, &hints, &servinfo)) != 0) {
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

	//freeaddrinfo(servinfo);

	if (p == NULL) {
		printf("Failed to open TCP socket.\n");
		return -1;
	}

	return 0;
}

int NetSocket::waitForConnection()
{
	if (hints.ai_socktype == SOCK_DGRAM) {
		return (this->waitForConnectionUDP());
	}
	else if (hints.ai_socktype == SOCK_STREAM) {
		return (this->waitForConnectionTCP());
	}
	else {
		printf("ERROR: NetSocket::waitForConnection unsupported socket type (UDP and TCP supported).\n");
		return -1;
	}
}

int NetSocket::waitForConnectionUDP()
{
	char init_msg[8], s[INET6_ADDRSTRLEN];
	int rv;

	// Wait to receive initial message and store server address info. BLOCKING
	printf("Waiting for UDP handshake on port %s...\n", port);
	addr_len = sizeof(their_addr);
	rv = recvfrom(socket_fd, init_msg, sizeof(init_msg), NULL, (struct sockaddr *)&their_addr, &addr_len);

	if (rv == SOCKET_ERROR) {
		printf("ERROR: recvfrom error. WSAError: %s/n", WSAGetLastError());
		freeaddrinfo(servinfo);
		return -1;
	}
		
	inet_ntop(hints.ai_family, get_in_addr((struct sockaddr*)&their_addr), s, sizeof s);
	printf("UDP handshake received from address: %s\n", s);

	return 0;
}

int NetSocket::waitForConnectionTCP()
{
	SOCKET temp_sock;
	char s[INET6_ADDRSTRLEN];
	int rv;

	if (listen(socket_fd, 10) == -1) {
		printf("ERROR: NetSocket::waitForConnectionTCP listen.\n");
		return -1;
	}

	printf("Waiting for TCP connection on port %s...\n", port);
	addr_len = sizeof(their_addr);
	temp_sock = accept(socket_fd, (struct sockaddr *)&their_addr, &addr_len);
	if (temp_sock == -1) {
		printf("ERROR: NetSocket::waitForConnectionTCP accept.\n");
		return -1;
	}

	/* Accepted client, stop listening for connections.*/
	closesocket(socket_fd);
	socket_fd = temp_sock;

	inet_ntop(hints.ai_family, get_in_addr((struct sockaddr*)&their_addr), s, sizeof s);
	printf("TCP connection received from address: %s\n", s);

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