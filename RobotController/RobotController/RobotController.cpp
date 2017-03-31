/****************************************************************************
*	RobotController.cpp
*
*	Main project file. 
*	Provides control inputs for a simulated robot over TCP/UDP sockets.
*	Intended for use with Gazebo simulator.
*
*	Author: Charles Hartsell
*	Date:	3-31-17
****************************************************************************/

#include "NetSocket.h"

#include "stdafx.h"

#define UDP_PORT "18423"
#define TCP_PORT "18424"
#define IP_ADDR "129.59.105.171"

int main(array<System::String ^> ^args)
{
	WSADATA wsaData;

	if ((WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
		printf("ERROR: WSAStartup failed.");
		return -1;
	}

	NetSocket UDP_Socket = NetSocket(UDP_PORT, IP_ADDR, SOCK_DGRAM);
	NetSocket TCP_Socket = NetSocket(TCP_PORT, IP_ADDR, SOCK_STREAM);
	UDP_Socket.openSocket();
	TCP_Socket.openSocket();
	TCP_Socket.waitForConnection();

	while (1) {
		Sleep(1000);
	}

	return 0;
}