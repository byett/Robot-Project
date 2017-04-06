/*****************************************************
*	NetSocket.h
*
*	Class for using TCP and UDP sockets.
*
*	Author: Charles Hartsell
*
*	Date:	3-31-17
*****************************************************/

#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "WS2_32.lib")

public class NetSocket
{
public:
	/* Public Functions */
	NetSocket(char* port, char* ip_address, int socktype);
	NetSocket(char* port, int socktype);
	NetSocket(int socktype);
	~NetSocket();

	int openSocket();
	int waitForConnection();
	int Send(char* msg, int msg_len);
	int Recv(char* buf, int* buf_len);
	void* get_in_addr(struct sockaddr *sa);

	/* Public Variables */

private:
	/* Private Functions */
	int openUDPSocket();
	int openTCPSocket();
	int waitForConnectionUDP();
	int waitForConnectionTCP();

	/* Private Variables */
	char port[INET6_ADDRSTRLEN], ip_address[INET6_ADDRSTRLEN];
	struct sockaddr_storage their_addr, client_addr;
	socklen_t addr_len, client_addr_len;
	struct addrinfo *servinfo, hints;
	SOCKET socket_fd = INVALID_SOCKET;
};

