/*********************************************************************************
*	StateMachine.cpp
*
*	Basic Finite State Machine class for implementing Robot Controller logic.
*
*	Author: Charles Hartsell
*
*	Date:	4-7-17
*********************************************************************************/

#include "StateMachine.h"

#include <string>

StateMachine::StateMachine()
{
	/* Initial FSM State */
	currentState = STOP_STATE;
	machineMode = AUTO_MODE;

	/* Variable Initilizations */
	latestUserCmd = NULL_CMD;
	outputCmd = NULL_CMD;
	userCmdArg = 0.0;
	outputCmdArg = 0.0;
	memset(&sensorData, 0x00, sizeof(sensorData));
}

int StateMachine::getCurrentState()
{
	return (this->currentState);
}

void StateMachine::sensorUpdate(int id, double value)
{
	sensorData.sensor_ranges[id % GAZEBO_SENSOR_BASE] = value;
}

void StateMachine::sensorUpdateAll(gazeboSensorData newData)
{
	sensorData = newData;
}

void StateMachine::userCmd(int id, double arg)
{
	if (id == MANUAL_MODE) machineMode = MANUAL_MODE;
	else if (id == AUTO_MODE) machineMode = AUTO_MODE;
	else {
		latestUserCmd = id;
		userCmdArg = arg;
	}
}

int StateMachine::getOutputCmd(int* id, double* arg)
{
	if (outputCmd == NULL_CMD) return -1;

	*id = outputCmd;
	*arg = outputCmdArg;
	outputCmd = NULL_CMD;
	outputCmdArg = 0.0;

	return 0;
}

int StateMachine::stepMachine()
{
	/*	State machine behavior defined in MATLAB Simulink file 
	 *	Current located at $ROBOT_PROJECT_HOME/fsm_for_robot_project.slx
	 *	Evaluated starting with highest level of Heirarchy, then moving down
	 */

	/* Top-level of Hierarchy */
	switch (machineMode) {
	case MANUAL_MODE:
		if (latestUserCmd == AUTO_MODE) {
			machineMode = AUTO_MODE;
			currentState = STOP_STATE;
		}
		break;
	case AUTO_MODE:
		if (latestUserCmd == MANUAL_MODE) {
			machineMode = MANUAL_MODE;
			currentState = STOP_STATE;
		}
		break;
	default:
		printf("ERROR: Unknown FSM machineMode (AUTO or MANUAL supported).\n");
		return -1;
	}

	/* Mid-level of Hierarchy */
	switch (currentState) {
	case STOP_STATE:
	case STOP_L_STATE:
	case STOP_R_STATE:
		if (latestUserCmd == FORWARD_CMD) {
			currentState = FORWARD_STATE;
		}
		else if (latestUserCmd == REVERSE_CMD) {
			currentState = REVERSE_STATE;
		}
		break;
	case FORWARD_STATE:
	case FORWARD_L_STATE:
	case FORWARD_R_STATE:
		if (latestUserCmd == STOP_CMD) {
			currentState = STOP_STATE;
		}
		else if (latestUserCmd == REVERSE_CMD) {
			currentState = REVERSE_STATE;
		}
		break;
	case REVERSE_STATE:
	case REVERSE_L_STATE:
	case REVERSE_R_STATE:
		if (latestUserCmd == STOP_CMD) {
			currentState = STOP_STATE;
		}
		else if (latestUserCmd == FORWARD_CMD) {
			currentState = FORWARD_STATE;
		}
		break;
	default:
		printf("ERROR: Unknown FSM State.\n");
		return -1;
	}

	/* Lowest level of Hierarchy */
	switch (currentState) {
	case STOP_STATE:
		if (latestUserCmd == TURN_L_CMD) {
			currentState = STOP_L_STATE;
		}
		else if (latestUserCmd == TURN_R_CMD) {
			currentState = STOP_R_STATE;
		}
		break;
	case STOP_L_STATE:
		if (latestUserCmd == STOP_CMD) {
			currentState = STOP_STATE;
		}
		else if (latestUserCmd == TURN_R_CMD) {
			currentState = STOP_R_STATE;
		}
		break;
	case STOP_R_STATE:
		if (latestUserCmd == STOP_CMD) {
			currentState = STOP_STATE;
		}
		else if (latestUserCmd == TURN_L_CMD) {
			currentState = STOP_L_STATE;
		}
		break;
	case FORWARD_STATE:
		if (latestUserCmd == TURN_L_CMD) {
			currentState = FORWARD_L_STATE;
		}
		else if (latestUserCmd == TURN_R_CMD) {
			currentState = FORWARD_R_STATE;
		}
		break;
	case FORWARD_L_STATE:
		if (latestUserCmd == FORWARD_CMD) {
			currentState = FORWARD_STATE;
		}
		else if (latestUserCmd == TURN_R_CMD) {
			currentState = FORWARD_R_STATE;
		}
		break;
	case FORWARD_R_STATE:
		if (latestUserCmd == FORWARD_CMD) {
			currentState = FORWARD_STATE;
		}
		else if (latestUserCmd == TURN_L_CMD) {
			currentState = FORWARD_L_STATE;
		}
		break;
	case REVERSE_STATE:
		if (latestUserCmd == TURN_L_CMD) {
			currentState = REVERSE_L_STATE;
		}
		else if (latestUserCmd == TURN_R_CMD) {
			currentState = REVERSE_R_STATE;
		}
		break;
	case REVERSE_L_STATE:
		if (latestUserCmd == FORWARD_CMD) {
			currentState = FORWARD_STATE;
		}
		else if (latestUserCmd == TURN_R_CMD) {
			currentState = FORWARD_R_STATE;
		}
		break;
	case REVERSE_R_STATE:
		if (latestUserCmd == REVERSE_CMD) {
			currentState = REVERSE_STATE;
		}
		else if (latestUserCmd == TURN_L_CMD) {
			currentState = REVERSE_L_STATE;
		}
		break;
	default:
		printf("ERROR: Unknown FSM state.\n");
		return -1;
	}

	latestUserCmd = NULL_CMD;
	return 0;
}