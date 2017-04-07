/****************************************************************************
*	RobotController.cpp
*
*	Main project file. 
*	Provides control inputs for a simulated robot over TCP/UDP sockets.
*	Intended for use with Gazebo simulator.
*
*	Author: Charles Hartsell
*			Ben Yett
*
*	Date:	3-31-17
****************************************************************************/

#include "NetSocket.h"
#include "Gesture.h"
#include "StateMachine.h"
#include "GazeboDefs.h"

#include "stdafx.h"

#define UDP_PORT "18423"
#define TCP_PORT "18424"
#define IP_ADDR "129.59.105.171"

#define MUTEX_WAIT_TIMEOUT_MS		500
#define THREAD_WAIT_TIMEOUT_MS		500

typedef struct {
	bool	threadShutdown;
	bool	new_msg;
	int		msg_len;
	char	*msg_p;
	HANDLE	mutex;
}threadSharedItems;

/* Gesture recognition thread function declaration */
DWORD WINAPI gestureThreadFunction(LPVOID lpParam);

/* Gazebo Listener thread function declaration */
DWORD WINAPI listenThreadFunction(LPVOID lpParam);

/* Helper functions */
int shutdownThread(HANDLE thread, threadSharedItems *t_items);


int main(/*array<System::String ^> ^args*/)
{
	WSADATA				wsaData;
	HANDLE				gestureThread, listenerThread;
	DWORD				gestureThreadID, listenerThreadID;
	threadSharedItems	*gestureShared, *listenerShared;
	gazeboSensorData	sensorData;

	char		buf[sizeof(int) + sizeof(double)];
	int			cmd_id;
	double		arg;

	/* Allocate and initilize memory */
	gestureShared	= (threadSharedItems*)malloc(sizeof(threadSharedItems));
	if (gestureShared == NULL) {
		printf("ERROR: Failed to allocate memory for gestureShared.\n");
		ExitProcess(1);
	}

	listenerShared	= (threadSharedItems*)malloc(sizeof(threadSharedItems));
	if (listenerShared == NULL) {
		free(gestureShared);
		printf("ERROR: Failed to allocate memory for listenerShared.\n");
		ExitProcess(1);
	}

	memset(gestureShared, 0x00, sizeof(threadSharedItems));
	memset(listenerShared, 0x00, sizeof(threadSharedItems));
	memset(&sensorData, 0x00, sizeof(gazeboSensorData));
	cmd_id = REVERSE_CMD;
	arg = 0.0;

	/* Create thread mutexes */
	gestureShared->mutex = CreateMutex(
		NULL,						// default security attributes
		FALSE,						// initially not owned
		NULL);						// unnamed mutex
	listenerShared->mutex = CreateMutex(
		NULL,						// default security attributes
		FALSE,						// initially not owned
		NULL);						// unnamed mutex

	if (gestureShared->mutex == NULL)
	{
		printf("CreateMutex error: %d\n", GetLastError());
		ExitProcess(2);
	}
	if (listenerShared->mutex == NULL)
	{
		printf("CreateMutex error: %d\n", GetLastError());
		ExitProcess(2);
	}

	/* Spawn gesture recognition thread */
	gestureThread = CreateThread(
		NULL,								// default security attributes
		0,									// use default stack size  
		gestureThreadFunction,				// thread function name
		(LPVOID)gestureShared,				// argument to thread function 
		0,									// use default creation flags 
		&gestureThreadID);					// returns the thread identifier 

	if (gestureThread == NULL) {
		printf("ERROR: Failed to spawn gesture recognition thread.\n");
		ExitProcess(3);
	}

	/* Startup WSA */
	if ((WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
		printf("ERROR: WSAStartup failed.");
		ExitProcess(1);
	}

	/* Open TCP Socket for command communication with Gazebo Interface. Block until handshake received. */
	NetSocket* TCP_Socket = new NetSocket(TCP_PORT, IP_ADDR, SOCK_STREAM);
	TCP_Socket->openSocket();
	TCP_Socket->waitForConnection();

	/* Spawn Gazebo Listener thread */
	listenerThread = CreateThread(
		NULL,								// default security attributes
		0,									// use default stack size  
		listenThreadFunction,				// thread function name
		(LPVOID)listenerShared,				// argument to thread function 
		0,									// use default creation flags 
		&listenerThreadID);					// returns the thread identifier 

	if (listenerThread == NULL) {
		printf("ERROR: Failed to spawn Gazebo listener thread.\n");
		ExitProcess(3);
	}

	/* Main task loop */
	while (1) {
		Sleep(1000);

		/* Lock mutex on shared data and read latest data update */
		WaitForSingleObject(listenerShared->mutex, INFINITE);
		memcpy(&sensorData, listenerShared->msg_p, sizeof(gazeboSensorData));
		ReleaseMutex(listenerShared->mutex);

		printf("wall_range: %f\tleft_range: %f\n", sensorData.sensor_ranges[WALL_ID % GAZEBO_SENSOR_BASE], sensorData.sensor_ranges[LEFT_ID % GAZEBO_SENSOR_BASE]);

		if (cmd_id == REVERSE_CMD) cmd_id = FORWARD_CMD;
		else cmd_id = REVERSE_CMD;
		memcpy(&buf[0], &cmd_id, sizeof(cmd_id));
		memcpy(&buf[sizeof(cmd_id)], &arg, sizeof(arg));
		printf("Sending command.\n");
		if (TCP_Socket->Send(buf, sizeof(buf)) == -1) {
			printf("TCP Send error.\n");
		}
	}

	/* Shutdown */
	shutdownThread(gestureThread, gestureShared);
	shutdownThread(listenerThread, listenerShared);
	free(gestureShared);
	free(listenerShared);
	WSACleanup();

	return 0;
}

/* Gesture recognition thread function definition */
DWORD WINAPI gestureThreadFunction(LPVOID lpParam) 
{
	threadSharedItems *gestureShared;
	int threadShutdown, gesture_msg;

	gestureShared = (threadSharedItems*)lpParam;
	threadShutdown = FALSE;

	/* Configure shared data structure */
	WaitForSingleObject(gestureShared->mutex, INFINITE);
	gestureShared->msg_len = sizeof(int);
	gestureShared->msg_p = (char*)&gesture_msg;
	ReleaseMutex(gestureShared->mutex);

	/* Spawn Gesture class and initilize Kinect*/
	Gesture* gesture = new Gesture();
	if ((gesture->CreateFirstConnected()) != S_OK) {
		printf("ERROR: Failed to link with Kinect. Stopping gesture recognition thread.\n");
		return -1;
	}
	else printf("Linked with Kinect.\n");

	while ( !threadShutdown ) {
		gesture->Update();

		/* Lock mutex on shared data. Check if thread has been told to shutdown */
		WaitForSingleObject(gestureShared->mutex, INFINITE);
		/* CODE TO PASS GESTURE MESSAGES GOES HERE */
		threadShutdown = gestureShared->threadShutdown;
		ReleaseMutex(gestureShared->mutex);
	}

	return 0;
}

/* Gazebo Listener thread function definition */
DWORD WINAPI listenThreadFunction(LPVOID lpParam)
{
	threadSharedItems	*listenerShared;
	gazeboSensorData	*sensorData;

	int		threadShutdown, data_id, buf_len, data_index;
	double	data_value;
	char	buf[GAZEBO_DATA_MSG_SIZE];

	listenerShared = (threadSharedItems*)lpParam;
	threadShutdown = FALSE;

	sensorData = (gazeboSensorData*)malloc(sizeof(gazeboSensorData));
	if (sensorData == NULL) {
		printf("ERROR: Failed to allocate memory for sensorData. Stopping listener thread.\n");
		return -1;
	}
	memset(sensorData, 0x00, sizeof(gazeboSensorData));

	/* Configure shared data structure */
	WaitForSingleObject(listenerShared->mutex, INFINITE);
	listenerShared->msg_len = sizeof(gazeboSensorData);
	listenerShared->msg_p = (char*)sensorData;
	ReleaseMutex(listenerShared->mutex);

	/* Open UDP Socket for listening to Gazebo data messages */
	NetSocket* UDP_Socket = new NetSocket(UDP_PORT, IP_ADDR, SOCK_DGRAM);
	UDP_Socket->openSocket();
	if ((UDP_Socket->waitForConnection()) == -1) {
		printf("ERROR: Failed to open UDP socket with Gazebo. Stopping listener thread.\n");
		return -1;
	}

	while (!threadShutdown) {
		buf_len = sizeof(buf);
		UDP_Socket->Recv(buf, &buf_len);

		/* Verify valid message length */
		if (buf_len == GAZEBO_DATA_MSG_SIZE) {
			memcpy(&data_id, &buf[0], sizeof(data_id));
			memcpy(&data_value, &buf[sizeof(data_id)], sizeof(data_value));

			/* Verify valid data_id */
			if ((data_id >= GAZEBO_SENSOR_BASE) || (data_id < (GAZEBO_SENSOR_BASE + GAZEBO_SENSOR_COUNT))) {
				data_index = data_id % GAZEBO_SENSOR_BASE;

				/* Lock mutex on shared data and Update Shared data structure with new sensor data value */
				WaitForSingleObject(listenerShared->mutex, INFINITE);
				sensorData->sensor_ranges[data_index] = data_value;
				ReleaseMutex(listenerShared->mutex);
			}
			else {
				printf("ERROR: Unknown data message ID (%d) received.\n", data_id);
			}
		}
		else if (buf_len == SOCKET_ERROR) printf("ERROR: UDP_Socket->Recv() failed.\n");
		else printf("ERROR: Received unknown message from Gazebo.\n");

		/* Check if thread has been told to shutdown */
		WaitForSingleObject(listenerShared->mutex, INFINITE);
		threadShutdown = listenerShared->threadShutdown;
		ReleaseMutex(listenerShared->mutex);
	}

	return 0;
}

int shutdownThread(HANDLE thread, threadSharedItems *t_items)
{
	int rv;

	rv = WaitForSingleObject(t_items->mutex, MUTEX_WAIT_TIMEOUT_MS);
	switch (rv) {
	case WAIT_OBJECT_0:
		t_items->threadShutdown = TRUE;
		ReleaseMutex(t_items->mutex);
		/* Wait for thread to exit gracefully */
		rv = WaitForSingleObject(thread, THREAD_WAIT_TIMEOUT_MS);
		switch (rv) {
		case WAIT_OBJECT_0:
			rv = 0;
			break;
		default:
			printf("ERROR: Thread did not shutdown in time after sending shutdown command.\n");
			TerminateThread(thread, -1);
			rv = -1;
			break;
		}
		break;
	case WAIT_TIMEOUT:
		printf("ERROR: Timeout when locking shared thread data mutex for thread shutdown. Terminating thread.\n");
		TerminateThread(thread, -1);
		rv = -1;
		break;
	default:
		printf("ERROR: Unknown error locking thread mutex. Terminating thread.\n");
		TerminateThread(thread, -1);
		rv = -1;
		break;
	}

	CloseHandle(t_items->mutex);
	CloseHandle(thread);

	return rv;
}