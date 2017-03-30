// KinectController.cpp : main project file.

#include "stdafx.h"

#include <string>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <iostream>
#include <cstdio>
#include <errno.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>


#define PORT "18463"
#define IP_ADDR "10.0.2.0"
#define MAXDATASIZE 100

// Lookup value ID's
#define WALL_ID 0xA0
#define LEFT_ID 0xA1
#define LEFTFRONT_ID 0xA2
#define RIGHT_ID 0xA3
#define RIGHTFRONT_ID 0xA4

// Command ID's
#define STOP_CMD 0xB0
#define FORWARD_CMD 0xB1
#define REVERSE_CMD 0xB2
#define TURN_CMD 0xB3
#define CMD_SUCCESS 0xBE
#define CMD_FAIL 0xBF


using namespace System;

int sockfd, optval;
struct addrinfo hints, *p;
int rv, init_msg;
char s[INET6_ADDRSTRLEN];
struct addrinfo *servinfo;
struct sockaddr_storage their_addr;
socklen_t addr_len;

// Networking Helper functions copied from "Beej's Networking Guide"
// get sockaddr, IPv4 or IPv6:
static void* get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int openSocket()
{
	// Taken from "Beej's Networking Guide"

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	optval = 1;

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof optval);
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			closesocket(sockfd);
			perror("client: bind");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	// Wait to receive initial message and store server address info
	recvfrom(sockfd, (char *)&init_msg, sizeof(int), 0, (struct sockaddr *)&their_addr, &addr_len);

	servinfo = p;
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
	Console::Write("TestInterface:Socket", "UDP Socket initialized with address: ");
	Console::WriteLine(s);

	return sockfd;
}

int sendCmd(int id, double arg)
{
	int status;
	char buf[sizeof(id) + sizeof(arg)];

	memcpy(&buf[0], &id, sizeof(id));
	memcpy(&buf[sizeof(id)], &arg, sizeof(arg));
	status = sendto(sockfd, (char *)buf, sizeof(buf), 0, (struct sockaddr*)&their_addr, addr_len);
	if (status == -1);//debugMsg("TestInterface:Socket", "Send Error: " << errno);

	return status;
}

void* listen()
{
	char buffer[MAXDATASIZE];
	std::string name;
	int id, size_id, size_range;
	double range, wall_range, left_range, leftfront_range, right_range, rightfront_range;

	size_id = sizeof(id);
	size_range = sizeof(range);

	// Recieve data over socket
	while (true) {
		recvfrom(sockfd, buffer, MAXDATASIZE, 0,
			(struct sockaddr *)&(their_addr), &(addr_len));
		memcpy(&id, &buffer[0], size_id);
		memcpy(&range, &buffer[size_id], size_range);

		if (id == CMD_SUCCESS) {
			//debugMsg("TestInterface:Socket", "Received cmd id: " << id << " Sending COMMAND_SUCCESS");
			//m_execInterface.handleCommandAck(last_cmd, COMMAND_SUCCESS);
		}
		else if (id == CMD_FAIL) {
			//debugMsg("TestInterface:Socket", "Received cmd id: " << id << " Sending COMMAND_FAILED");
			//m_execInterface.handleCommandAck(last_cmd, COMMAND_FAILED);
		}
		else {
			// check ID and set appropriate name
			switch (id) {
			case WALL_ID:
				name = "wall_sensor";
				wall_range = range;
				break;
			case LEFT_ID:
				name = "left_sensor";
				left_range = range;
				break;
			case LEFTFRONT_ID:
				name = "leftfront_sensor";
				leftfront_range = range;
				break;
			case RIGHT_ID:
				name = "right_sensor";
				right_range = range;
				break;
			case RIGHTFRONT_ID:
				name = "rightfront_sensor";
				rightfront_range = range;
				break;
			default:
				//debugMsg("TestInterface:Socket", "Recieved unknown sensor ID: " << id);
				break;
			}

			// Propigate change to PLEXIL
			//retval = (Value)range;
			//st = createState(name, EmptyArgs);
			//propagateValueChange(st, vector<Value>(1, retval));
		}
	}

	return NULL;
}

int main(array<System::String ^> ^args)
{
    Console::WriteLine(L"Hello World");
    return 0;
}