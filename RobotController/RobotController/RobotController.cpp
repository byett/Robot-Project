// RobotController.cpp : main project file.

#include "NetSocket.h"

#include "stdafx.h"

#define UDP_PORT "18423"
#define TCP_PORT "18424"
#define IP_ADDR "129.59.105.171"

int main(array<System::String ^> ^args)
{
	NetSocket UDP_Socket = NetSocket(UDP_PORT, IP_ADDR);
	NetSocket TCP_Socket = NetSocket(TCP_PORT, IP_ADDR);
	UDP_Socket.openSocket();

	while (1) {
		Sleep(1000);
	}

	return 0;
}