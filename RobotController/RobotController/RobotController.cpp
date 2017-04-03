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
#include "Gesture.h"

#include "stdafx.h"

#define UDP_PORT "18423"
#define TCP_PORT "18424"
#define IP_ADDR "129.59.105.171"

#define WALL_ID			0xA0
#define LEFT_ID			0xA1
#define LEFTFRONT_ID	0xA2
#define RIGHT_ID		0xA3
#define RIGHTFRONT_ID	0xA4

#define STOP_CMD		0xB0
#define FORWARD_CMD		0xB1
#define REVERSE_CMD		0xB2
#define TURN_CMD		0xB3

int main(/*array<System::String ^> ^args*/)
{
	WSADATA wsaData;
	char buf[sizeof(int) + sizeof(double)];
	int cmd_id;
	double arg;

	cmd_id = REVERSE_CMD;
	arg = 0.0;

	if ((WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
		printf("ERROR: WSAStartup failed.");
		return -1;
	}

	Gesture* gesture = new Gesture();
	gesture->CreateFirstConnected();

	int i;
	for (i = 0; i < 1; i++) {
		gesture->Update();
	}

	NetSocket* UDP_Socket = new NetSocket(UDP_PORT, IP_ADDR, SOCK_DGRAM);
	NetSocket* TCP_Socket = new NetSocket(TCP_PORT, IP_ADDR, SOCK_STREAM);
	UDP_Socket->openSocket();
	TCP_Socket->openSocket();
	TCP_Socket->waitForConnection();
	UDP_Socket->waitForConnection();

	while (1) {
		if (cmd_id == REVERSE_CMD) cmd_id = FORWARD_CMD;
		else cmd_id = REVERSE_CMD;
		memcpy(&buf[0], &cmd_id, sizeof(cmd_id));
		memcpy(&buf[sizeof(cmd_id)], &arg, sizeof(arg));
		printf("Sending command.\n");
		if (TCP_Socket->Send(buf, sizeof(buf)) == -1) {
			printf("TCP Send error.\n");
		}
		Sleep(1000);
	}

	return 0;
}