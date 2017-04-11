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

StateMachine::StateMachine()
{
	/* Initial FSM State */
	currentState = STOP_STATE;
	machineMode = AUTO_MODE;
	outputCmd = NULL_CMD;
	tickCount = 0;
}

int StateMachine::getCurrentState()
{
	return currentState;
}

void StateMachine::setInput(uint16_t input)
{
	externalInput = input;
}

int StateMachine::getOutputCmd()
{
	int temp;

	temp = outputCmd;
	outputCmd = NULL_CMD;

	return temp;
}

int StateMachine::stepMachine()
{
	/*	State machine behavior defined in MATLAB Simulink file 
	 *	Current located at $ROBOT_PROJECT_HOME/cps.slx
	 */

	tickCount++;
	switch (machineMode) {

	/* Manual Mode */
	case MANUAL_MODE:
		if (externalInput & AUTO_MODE_CMD_MASK) {
			machineMode = AUTO_MODE;
			currentState = AUTO_FORWARD_STATE;
			outputCmd = FORWARD_CMD;
		}
		else {
			switch (currentState) {

			}
		}
		break;

	/* Autonomous Mode */
	case AUTO_MODE:
		if (externalInput & MANUAL_MODE_CMD_MASK) {
			machineMode = MANUAL_MODE;
			currentState = STOP_STATE;
			outputCmd = STOP_CMD;
		}
		else {
			switch (currentState) {
			case AUTO_FORWARD_STATE:
				if (externalInput & (WALL_SENSOR_MASK | LEFT_SENSOR_MASK | RIGHT_SENSOR_MASK)) {
					if (externalInput & LEFT_SENSOR_MASK) left_sensor_tripped = true;
					currentState = AUTO_REVERSE_STATE;
					outputCmd = REVERSE_CMD;
					tickCount = 0;
				}
				else outputCmd = FORWARD_CMD;
				break;
			case AUTO_REVERSE_STATE:
				if (tickCount >= AUTO_AVOIDANCE_REVERSE_DELAY_TICKS ) {
					if (left_sensor_tripped) {
						currentState = AUTO_TURN_L_STATE;
						outputCmd = TURN_L_CMD;
						left_sensor_tripped = false;
					}
					else
					{
						currentState = AUTO_TURN_R_STATE;
						outputCmd = TURN_R_CMD;
					}
					tickCount = 0;
				}
				else outputCmd = REVERSE_CMD;
				break;
			case AUTO_TURN_L_STATE:
				if (tickCount >= AUTO_AVOIDANCE_TURN_DELAY_TICKS) {
					if (externalInput & (WALL_SENSOR_MASK | LEFT_SENSOR_MASK | RIGHT_SENSOR_MASK)) {
						currentState = AUTO_REVERSE_STATE;
						outputCmd = REVERSE_CMD;
					}
					else {
						currentState = AUTO_FORWARD_STATE;
						outputCmd = FORWARD_CMD;
					}
				}
				else outputCmd = TURN_L_CMD;
				break;
			case AUTO_TURN_R_STATE:
				if (tickCount >= AUTO_AVOIDANCE_TURN_DELAY_TICKS) {
					if (externalInput & (WALL_SENSOR_MASK | LEFT_SENSOR_MASK | RIGHT_SENSOR_MASK)) {
						currentState = AUTO_REVERSE_STATE;
						outputCmd = REVERSE_CMD;
					}
					else {
						currentState = AUTO_FORWARD_STATE;
						outputCmd = FORWARD_CMD;
					}
				}
				else outputCmd = TURN_R_CMD;
				break;
			}
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