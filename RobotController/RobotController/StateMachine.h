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
#include "StateMachineDefs.h"

#include <string>

/* States of machine */
#define NULL_STATE				0x00
#define FORWARD_STATE			0x01
#define FORWARD_L_STATE			0x02
#define FORWARD_R_STATE			0x03
#define STOP_STATE				0x04
#define STOP_L_STATE			0x05
#define STOP_R_STATE			0x06
#define REVERSE_STATE			0x07
#define REVERSE_L_STATE			0x08
#define REVERSE_R_STATE			0x09
#define AUTO_FORWARD_STATE		0x10
#define AUTO_REVERSE_STATE		0x11
#define AUTO_TURN_L_STATE		0x12
#define AUTO_TURN_R_STATE		0x13

#define AUTO_MODE				0x30
#define MANUAL_MODE				0x31

#define STATE_MACHINE_TICK_TIME_MS				100	
#define AUTO_AVOIDANCE_REVERSE_DELAY_TICKS		5
#define AUTO_AVOIDANCE_TURN_DELAY_TICKS			5

public class StateMachine 
{
public:
	/* Public Functions */
	StateMachine();
	int getCurrentState();
	void setInput(uint16_t input);
	int getOutputCmd();
	int stepMachine();

	/* Public Variables */

private:
	/* Private Functions */

	/* Private Variables */
	int currentState;
	int machineMode;
	int outputCmd;
	int tickCount;
	bool left_sensor_tripped;
	uint16_t externalInput;
};