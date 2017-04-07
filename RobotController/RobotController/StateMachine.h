/*********************************************************************************
*	StateMachine.h
*
*	Basic Finite State Machine class for implementing Robot Controller logic.
*
*	Author: Charles Hartsell
*
*	Date:	4-7-17
*********************************************************************************/

#pragma once

#include "GazeboDefs.h"

/* States of machine */
#define NULL_STATE			0x00
#define FORWARD_STATE		0x01
#define FORWARD_L_STATE		0x02
#define FORWARD_R_STATE		0x03
#define STOP_STATE			0x04
#define STOP_L_STATE		0x05
#define STOP_R_STATE		0x06
#define REVERSE_STATE		0x07
#define REVERSE_L_STATE		0x08
#define REVERSE_R_STATE		0x09

public class StateMachine 
{
public:
	/* Public Functions */
	StateMachine();
	int getCurrentState();
	void sensorUpdate(int id, double value);
	void sensorUpdateAll(gazeboSensorData newData);
	void userCmd(int id, double arg);
	int getOutputCmd(int* id, double* arg);
	int stepMachine();

	/* Public Variables */

private:
	/* Private Functions */

	/* Private Variables */
	int currentState;
	int machineMode;
	int latestUserCmd, outputCmd;
	double userCmdArg, outputCmdArg;
	gazeboSensorData sensorData;
};