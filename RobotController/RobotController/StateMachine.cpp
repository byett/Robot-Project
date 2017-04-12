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
	machineMode = MANUAL_MODE;
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
	/*	State machine behavior defined in MATLAB Simulink file.
	 *	Currently located at $ROBOT_PROJECT_HOME/cps.slx
	 *	For Manual mode of operation, the mid-level hierarchy is evaluated before the bottom-level.
	 *	This is a very tedious and ugly way to implement an FSM.
	 */

	tickCount++;
	outputCmd = NULL_CMD;

	switch (machineMode) {
	/* Manual Mode */
	case MANUAL_MODE:
		if (externalInput & AUTO_MODE_CMD_MASK) {
			machineMode = AUTO_MODE;
			currentState = AUTO_FORWARD_STATE;
			outputCmd = FORWARD_CMD;
			break;
		}
		else {
			/* Mid-level of Hierarchy */
			if (externalInput & (WALL_SENSOR_MASK | LEFT_SENSOR_MASK | RIGHT_SENSOR_MASK)) {
				printf("Obstacle Detected.\n");
				currentState = AVOID_OBSTACLE_STATE;
				outputCmd = REVERSE_CMD;
				break;
			}
			switch (currentState) {
			case STOP_STATE:
			case STOP_L_STATE:
			case STOP_R_STATE:
				if (externalInput & FORWARD_CMD_MASK) {
					currentState = FORWARD_STATE;
					outputCmd = FORWARD_CMD;
				}
				else if (externalInput & REVERSE_CMD_MASK) {
					currentState = REVERSE_STATE;
					outputCmd = REVERSE_CMD;
				}
				break;
			case FORWARD_STATE:
			case FORWARD_L_STATE:
			case FORWARD_R_STATE:
				if (externalInput & STOP_CMD_MASK) {
					currentState = STOP_STATE;
					outputCmd = STOP_CMD;
				}
				else if (externalInput & REVERSE_CMD_MASK) {
					currentState = REVERSE_STATE;
					outputCmd = REVERSE_CMD;
				}
				break;
			case REVERSE_STATE:
			case REVERSE_L_STATE:
			case REVERSE_R_STATE:
				if (externalInput & STOP_CMD_MASK) {
					currentState = STOP_STATE;
					outputCmd = STOP_CMD;
				}
				else if (externalInput & FORWARD_CMD_MASK) {
					currentState = FORWARD_STATE;
					outputCmd = FORWARD_CMD;
				}
				break;
			case AVOID_OBSTACLE_STATE:
				if ( !(externalInput & (WALL_SENSOR_MASK | LEFT_SENSOR_MASK | RIGHT_SENSOR_MASK)) ) {
					currentState = STOP_STATE;
					outputCmd = STOP_CMD;
				}
				else outputCmd = REVERSE_CMD;
				break;
			default:
				printf("ERROR: Unknown FSM State %d. Manual Mid-Level.\n", currentState);
				return -1;
			}

			/* If Mid-level hierarchy caused state transition, do not evaluate bottom level */
			if (outputCmd != NULL_CMD) break;

			/* Bottom level of Hierarchy */
			switch (currentState) {
			case STOP_STATE:
				if (externalInput & TURN_L_CMD_MASK) {
					currentState = STOP_L_STATE;
					outputCmd = TURN_L_CMD;
				}
				else if (externalInput & TURN_R_CMD_MASK) {
					currentState = STOP_R_STATE;
					outputCmd = TURN_R_CMD;
				}
				else outputCmd = STOP_CMD;
				break;
			case STOP_L_STATE:
				if (externalInput & (STOP_CMD_MASK | STOP_TURN_CMD_MASK)) {
					currentState = STOP_STATE;
					outputCmd = STOP_CMD;
				}
				else if (externalInput & TURN_R_CMD) {
					currentState = STOP_R_STATE;
					outputCmd = TURN_R_CMD;
				}
				else outputCmd = TURN_L_CMD;
				break;
			case STOP_R_STATE:
				if (externalInput & (STOP_CMD_MASK | STOP_TURN_CMD_MASK)) {
					currentState = STOP_STATE;
					outputCmd = STOP_CMD;
				}
				else if (externalInput & TURN_L_CMD) {
					currentState = STOP_L_STATE;
					outputCmd = TURN_L_CMD;
				}
				else outputCmd = TURN_R_CMD;
				break;
			case FORWARD_STATE:
				if (externalInput & TURN_L_CMD_MASK) {
					currentState = FORWARD_L_STATE;
					outputCmd = FORWARD_L_CMD;
				}
				else if (externalInput & TURN_R_CMD_MASK) {
					currentState = FORWARD_R_STATE;
					outputCmd = FORWARD_R_CMD;
				}
				else outputCmd = FORWARD_CMD;
				break;
			case FORWARD_L_STATE:
				if (externalInput & (FORWARD_CMD_MASK | STOP_TURN_CMD_MASK)) {
					currentState = FORWARD_STATE;
					outputCmd = FORWARD_CMD;
				}
				else if (externalInput & TURN_R_CMD) {
					currentState = FORWARD_R_STATE;
					outputCmd = FORWARD_R_CMD;
				}
				else outputCmd = FORWARD_L_CMD;
				break;
			case FORWARD_R_STATE:
				if (externalInput & (FORWARD_CMD_MASK | STOP_TURN_CMD_MASK)) {
					currentState = FORWARD_STATE;
					outputCmd = FORWARD_CMD;
				}
				else if (externalInput & TURN_L_CMD) {
					currentState = FORWARD_L_STATE;
					outputCmd = FORWARD_L_CMD;
				}
				else outputCmd = FORWARD_R_CMD;
				break;
			case REVERSE_STATE:
				if (externalInput & TURN_L_CMD_MASK) {
					currentState = REVERSE_L_STATE;
					outputCmd = REVERSE_L_CMD;
				}
				else if (externalInput & TURN_R_CMD_MASK) {
					currentState = REVERSE_R_STATE;
					outputCmd = REVERSE_R_CMD;
				}
				else outputCmd = REVERSE_CMD;
				break;
			case REVERSE_L_STATE:
				if (externalInput & (REVERSE_CMD_MASK | STOP_TURN_CMD_MASK)) {
					currentState = REVERSE_STATE;
					outputCmd = REVERSE_CMD;
				}
				else if (externalInput & TURN_R_CMD_MASK) {
					currentState = REVERSE_R_STATE;
					outputCmd = REVERSE_R_CMD;
				}
				else outputCmd = REVERSE_L_CMD;
				break;
			case REVERSE_R_STATE:
				if (externalInput & (REVERSE_CMD_MASK | STOP_TURN_CMD_MASK)) {
					currentState = REVERSE_STATE;
					outputCmd = REVERSE_CMD;
				}
				else if (externalInput & TURN_L_CMD_MASK) {
					currentState = REVERSE_L_STATE;
					outputCmd = REVERSE_L_CMD;
				}
				else outputCmd = REVERSE_R_CMD;
				break;
			default:
				printf("ERROR: Unknown FSM State %d. Manual Bottom-Level.\n", currentState);
				return -1;
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
			default:
				printf("ERROR: Unknown FSM State %d. Auto mode.\n", currentState);
				return -1;
			}
		}
		break;
	default:
		printf("ERROR: Unknown FSM machineMode (AUTO or MANUAL supported).\n");
		return -1;
	}

	externalInput = 0;
	return 0;
}