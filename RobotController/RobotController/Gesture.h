/*****************************************************
*	Gesture.h
*
*	Kinect Gesture Recognition
*
*	Author: Charles Hartsell
*			Ben Yett
*
*	Date:	4-3-17
*****************************************************/

#pragma once

#include <windows.h>
#include <NuiApi.h>

/* Gesture Turning parameters */
#define GESTURE_MAX_TURN_L PI/2
#define GESTURE_MAX_TURN_R -PI/2

public class Gesture
{
public:
	/* Public Functions */
	Gesture();
	~Gesture();
	void Update();
	int getUserInput();
	double getUserArg();

	/// <summary>
	/// Create the first connected Kinect found 
	/// </summary>
	/// <returns>S_OK on success, otherwise failure code</returns>
	HRESULT                 CreateFirstConnected();

	/* Public Variables */

private:
	/* Private Functions */
	void clearall();

	/// <summary>
	/// Handle new skeleton data
	/// </summary>
	void                    ProcessSkeleton();

	/// <summary>
	/// Gesture recognition using skeleton data
	/// </summary>
	void determine_gesture(const NUI_SKELETON_DATA & skeleton);
	

	/* Private Variables */
	int stop, forwarda, forwardb, backwarda, backwardb, autoa, autob, mana, manb, turncount;
	int user_input;
	double user_arg, result;

	INuiSensor*             m_pNuiSensor;

	HANDLE                  m_pSkeletonStreamHandle;
	HANDLE                  m_hNextSkeletonEvent;

};
