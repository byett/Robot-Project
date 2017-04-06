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

public class Gesture
{
public:
	/* Public Functions */
	Gesture();
	~Gesture();
	void Update();

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
	double result;

	INuiSensor*             m_pNuiSensor;

	HANDLE                  m_pSkeletonStreamHandle;
	HANDLE                  m_hNextSkeletonEvent;

};
