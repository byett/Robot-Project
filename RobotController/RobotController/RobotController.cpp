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
#include "StateMachineDefs.h"

#include "stdafx.h"

#define UDP_PORT "18423"
#define TCP_PORT "18424"

#define MUTEX_WAIT_TIMEOUT_MS		500
#define THREAD_WAIT_TIMEOUT_MS		500

typedef struct threadSharedItems{
	bool	threadShutdown;
	bool	new_msg;
	int		msg_len;
	char	*msg_p;
	HANDLE	mutex;
}threadSharedItems;

typedef struct gestureData {
	uint16_t		user_cmd;
	double	arg;
}gestureData;

/* Gesture recognition thread function declaration */
DWORD WINAPI gestureThreadFunction(LPVOID lpParam);

/* Gazebo Listener thread function declaration */
DWORD WINAPI listenThreadFunction(LPVOID lpParam);

/* Helper functions */
int shutdownThread(HANDLE thread, threadSharedItems *t_items);
uint16_t DEBUG_GetUserInputCMDLine();
void DEBUG_PrintUserCMD(uint16_t input);
void DEBUG_PrintCMD(int cmd_id, double cmd_arg);


int main(/*array<System::String ^> ^args*/)
{
	/* Thread Specific variables */
	WSADATA				wsaData;
	HANDLE				gestureThread, listenerThread;
	DWORD				gestureThreadID, listenerThreadID;
	threadSharedItems	*gestureShared, *listenerShared;
	

	char		buf[GAZEBO_CMD_MSG_SIZE];
	int			cmd_id;
	double		turn_angle;
	uint16_t	machineInput;
	StateMachine *FSM = new StateMachine();

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
	cmd_id = NULL_CMD;
	machineInput = 0;
	turn_angle = 0.0;

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
	NetSocket* TCP_Socket = new NetSocket(TCP_PORT, SOCK_STREAM);
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
		Sleep( STATE_MACHINE_TICK_TIME_MS );

		/* Lock mutex on listener shared data and read latest data update */
		WaitForSingleObject(listenerShared->mutex, INFINITE);
		if (listenerShared->new_msg == TRUE) {
			machineInput |= *((uint16_t*)(listenerShared->msg_p));
		}
		listenerShared->new_msg = FALSE;
		ReleaseMutex(listenerShared->mutex);

		/* Lock mutex on gesture shared data and read latest user input */
		WaitForSingleObject(gestureShared->mutex, INFINITE);
		if (gestureShared->new_msg == TRUE) {
			machineInput |= ((gestureData*)(gestureShared->msg_p))->user_cmd;
			turn_angle = ((gestureData*)(gestureShared->msg_p))->arg;
			((gestureData*)(gestureShared->msg_p))->user_cmd = NULL_CMD;
			((gestureData*)(gestureShared->msg_p))->arg = 0.0;
		}
		gestureShared->new_msg = FALSE;
		ReleaseMutex(gestureShared->mutex);

		/************ DEBUG *************
		if (machineInput & (STOP_CMD_MASK | FORWARD_CMD_MASK | REVERSE_CMD_MASK | TURN_L_CMD_MASK | TURN_R_CMD_MASK | MANUAL_MODE_CMD_MASK | AUTO_MODE_CMD_MASK))
			DEBUG_PrintUserCMD(machineInput);
		*/

		/* Send input to FSM, step, and get output */
		FSM->setInput(machineInput, turn_angle);
		FSM->stepMachine();
		cmd_id = FSM->getOutputCmd();
		machineInput = NULL_CMD_MASK;

		/************ DEBUG *************
		machineInput = FSM->getCurrentState();
		printf("cmd: %d\tstate: %d\n", cmd_id, machineInput);
		machineInput = NULL_CMD_MASK;*/

		/************ DEBUG *************
		if (cmd_id != NULL_CMD)
			DEBUG_PrintCMD(cmd_id, turn_angle);
		*/

		/* Send command and argument (as applicable) to gazeboInterface */
		if (cmd_id != NULL_CMD){
			memcpy(&buf[0], &cmd_id, sizeof(cmd_id));
			if ((FSM->getCurrentState()) == AUTO_TURN_L_STATE){
				turn_angle = GESTURE_MAX_TURN_L;
			}
			else if ((FSM->getCurrentState()) == AUTO_TURN_R_STATE){
				turn_angle = GESTURE_MAX_TURN_R;
			}
			memcpy(&buf[sizeof(cmd_id)], &turn_angle, sizeof(turn_angle));
			if (TCP_Socket->Send(buf, sizeof(buf)) == -1) {
				printf("TCP Send error.\n");
			}
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
	gestureData *inputData;
	int threadShutdown;
	uint16_t userInput;
	double arg;

	gestureShared = (threadSharedItems*)lpParam;
	threadShutdown = FALSE;

	inputData = (gestureData*)malloc(sizeof(gestureData));
	if (inputData == NULL) {
		printf("ERROR: Failed to allocate memory for inputData. Stopping gesture thread.\n");
		return -1;
	}
	memset(inputData, 0x00, sizeof(gestureData));

	/* Spawn Gesture class and initilize Kinect*/
	Gesture* gesture = new Gesture();
	if ((gesture->CreateFirstConnected()) != S_OK) {
		printf("ERROR: Failed to link with Kinect. Stopping gesture recognition thread.\n");
		return -1;
	}
	else printf("Linked with Kinect.\n");

	/* Configure shared data structure */
	WaitForSingleObject(gestureShared->mutex, INFINITE);
	gestureShared->msg_len = sizeof(gestureData);
	gestureShared->msg_p = (char*)inputData;
	ReleaseMutex(gestureShared->mutex);

	while ( !threadShutdown ) {
		gesture->Update();
		userInput = gesture->getUserInput();
		arg = gesture->getUserArg();

		/* Lock mutex on shared data. Update inputData if necessary and check if thread has been told to shutdown */
		WaitForSingleObject(gestureShared->mutex, INFINITE);
		if ((userInput & ~(STOP_TURN_CMD_MASK)) != NULL_CMD_MASK) {
			gestureShared->new_msg = TRUE;
			inputData->user_cmd = userInput;
			inputData->arg = arg;
		}
		else if (userInput & STOP_TURN_CMD_MASK){
			gestureShared->new_msg = TRUE;
			inputData->user_cmd |= STOP_TURN_CMD_MASK;
		}
		threadShutdown = gestureShared->threadShutdown;
		ReleaseMutex(gestureShared->mutex);
	}

	return 0;
}

/* Gazebo Listener thread function definition */
DWORD WINAPI listenThreadFunction(LPVOID lpParam)
{
	threadSharedItems	*listenerShared;
	gazeboSensorData	sensorData;

	int			threadShutdown, data_id, buf_len, data_index;
	uint16_t	sensorMask, sharedMask;
	double		data_value;
	char		buf[GAZEBO_DATA_MSG_SIZE];

	listenerShared = (threadSharedItems*)lpParam;
	threadShutdown = FALSE;
	memset(&sensorData, 0x00, sizeof(sensorData));

	/* Open UDP Socket for listening to Gazebo data messages */
	NetSocket* UDP_Socket = new NetSocket(UDP_PORT, SOCK_DGRAM);
	UDP_Socket->openSocket();
	if ((UDP_Socket->waitForConnection()) == -1) {
		printf("ERROR: Failed to open UDP socket with Gazebo. Stopping listener thread.\n");
		return -1;
	}

	/* Configure shared data structure */
	WaitForSingleObject(listenerShared->mutex, INFINITE);
	listenerShared->msg_len = sizeof(uint16_t);
	listenerShared->msg_p = (char*)&sharedMask;
	ReleaseMutex(listenerShared->mutex);

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
				sensorData.sensor_ranges[data_index] = data_value;
			}
			else {
				printf("ERROR: Unknown data message ID (%d) received.\n", data_id);
			}
		}
		else if (buf_len == SOCKET_ERROR) printf("ERROR: UDP_Socket->Recv() failed.\n");
		else printf("ERROR: Received unknown message from Gazebo.\n");

		/* Check if any sensor has tripped (detects danger condition) */
		sensorMask = 0;
		if (sensorData.sensor_ranges[WALL_ID % GAZEBO_SENSOR_BASE] < WALL_SENSOR_TRIP_RANGE) 
		{
			sensorMask |= WALL_SENSOR_MASK;
		}
		if (sensorData.sensor_ranges[LEFT_ID % GAZEBO_SENSOR_BASE] > TILT_SENSOR_TRIP_RANGE ||
			sensorData.sensor_ranges[LEFTFRONT_ID % GAZEBO_SENSOR_BASE] > TILT_SENSOR_TRIP_RANGE ) 
		{
			sensorMask |= LEFT_SENSOR_MASK;
		}
		if (sensorData.sensor_ranges[RIGHT_ID % GAZEBO_SENSOR_BASE] > TILT_SENSOR_TRIP_RANGE ||
			sensorData.sensor_ranges[RIGHTFRONT_ID % GAZEBO_SENSOR_BASE] > TILT_SENSOR_TRIP_RANGE)
		{
			sensorMask |= RIGHT_SENSOR_MASK;
		}

		/* Update shared data and check if thread has been told to shutdown. */
		WaitForSingleObject(listenerShared->mutex, INFINITE);
		listenerShared->new_msg = TRUE;
		sharedMask = sensorMask;
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

uint16_t DEBUG_GetUserInputCMDLine()
{
	char buf[10];
	uint16_t input;

	printf("User Input: ");
	std::cin.getline(buf, 10);
	input = atoi(buf);

	return input;
}

void DEBUG_PrintUserCMD(uint16_t input)
{
	printf("User Input:\n");
	if (input & STOP_CMD_MASK)
		printf("\tSTOP\n");
	if (input & FORWARD_CMD_MASK)
		printf("\tFORWARD\n");
	if (input & REVERSE_CMD_MASK)
		printf("\tREVERSE\n");
	if (input & TURN_L_CMD_MASK)
		printf("\tTURN_L\n");
	if (input & TURN_R_CMD_MASK)
		printf("\tTURN_R\n");
	if (input & AUTO_MODE_CMD_MASK)
		printf("\tAUTO MODE\n");
	if (input & MANUAL_MODE_CMD_MASK)
		printf("\tMANUAL MODE\n");

	printf("\n");
}

void DEBUG_PrintCMD(int cmd_id, double cmd_arg)
{
	printf("Sending Command:\t");
	switch (cmd_id){
	case STOP_CMD:
		printf("STOP\n");
		break;
	case FORWARD_CMD:
		printf("FORWARD\n");
		break;
	case REVERSE_CMD:
		printf("REVERSE\n");
		break;
	case TURN_L_CMD:
		printf("TURN_L\t turn_angle: %f\n", cmd_arg);
		break;
	case TURN_R_CMD:
		printf("TURN_R\t turn_angle: %f\n", cmd_arg);
		break;
	case FORWARD_L_CMD:
		printf("FORWARD_L\t turn_angle: %f\n", cmd_arg);
		break;
	case FORWARD_R_CMD:
		printf("FORWARD_R\t turn_angle: %f\n", cmd_arg);
		break;
	case REVERSE_L_CMD:
		printf("REVERSE_L\t turn_angle: %f\n", cmd_arg);
		break;
	case REVERSE_R_CMD:
		printf("REVERSE_R\t turn_angle: %f\n", cmd_arg);
		break;
	case AUTO_MODE_CMD:
		printf("AUTO_MODE\n");
		break;
	case MANUAL_MODE_CMD:
		printf("MANUAL_MODE\n");
		break;
	default:
		printf("ERROR: RobotContoller DEBUG_PrintCMD Unknown command ID.\n");
		break;
	}

	printf("\n");
}