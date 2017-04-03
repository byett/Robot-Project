/*****************************************************
*	Gesture.h
*
*	Kinect Gesture Recognition
*
*	Author: Charles Hartsell
*			Ben Yett
*	Date:	4-3-17
*****************************************************/

#pragma once

#include "resource.h"
#include <windows.h>
#include <NuiApi.h>

public class Gesture
{
public:
	/* Public Functions */
	Gesture();
	~Gesture();

	/// <summary>
	/// Create the first connected Kinect found 
	/// </summary>
	/// <returns>S_OK on success, otherwise failure code</returns>
	HRESULT                 CreateFirstConnected();

	/// <summary>
	/// Handle new skeleton data
	/// </summary>
	void                    ProcessSkeleton();

	// Gesture Recognition
	void determine_gesture(const NUI_SKELETON_DATA & skeleton);

	void Update();

	/* Public Variables */

private:
	/* Private Functions */
	void clearall();
	

	/* Private Variables */
	int stop, forwarda, forwardb, backwarda, backwardb, autoa, autob, mana, manb, turncount;
	double result;

	INuiSensor*             m_pNuiSensor;

	HANDLE                  m_pSkeletonStreamHandle;
	HANDLE                  m_hNextSkeletonEvent;

};
