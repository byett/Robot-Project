/*****************************************************
*	Gesture.cpp
*
*	Kinect Gesture Recognition
*
*	Author: Charles Hartsell
*			Ben Yett
*
*	Date:	4-3-17
*****************************************************/

#include "stdafx.h"

#include "Gesture.h"
#include "StateMachineDefs.h"
#include <strsafe.h>
#include <math.h>
#include <windows.h>
#include <NuiApi.h>
#include <NuiSkeleton.h>
#include <iostream>    
#include <stdlib.h>
#include <string>
#include <fstream>

#define WAIT_FOR_FRAME_TIME_MS	50

Gesture::Gesture() :
	m_pNuiSensor(NULL),
	m_hNextSkeletonEvent(INVALID_HANDLE_VALUE),
	m_pSkeletonStreamHandle(INVALID_HANDLE_VALUE)
{
	clearall();
	user_input = NULL_CMD_MASK;
	user_arg = 0.0;
}

Gesture::~Gesture()
{
	if (m_pNuiSensor)
	{
		m_pNuiSensor->NuiShutdown();
	}

	if (m_hNextSkeletonEvent && (m_hNextSkeletonEvent != INVALID_HANDLE_VALUE))
	{
		CloseHandle(m_hNextSkeletonEvent);
	}
}

/* Basic functions for external access to private variables 
 *  NOTE:	The values of user_input and user_arg are reset each time determine_gesture() function is called
 *			These values should be read after every call of Update() before calling Update() again to ensure no input is missed. 		 
 */
int Gesture::getUserInput()
{
	return user_input;
}
double Gesture::getUserArg()
{
	return user_arg;
}

void Gesture::ProcessSkeleton()
{
	NUI_SKELETON_FRAME skeletonFrame = { 0 };
	int i;

	HRESULT hr = m_pNuiSensor->NuiSkeletonGetNextFrame(0, &skeletonFrame);
	if (FAILED(hr))
	{
		printf("ProcessSkeleton failed.\n");
		return;
	}

	/* smooth out the skeleton data */
	m_pNuiSensor->NuiTransformSmooth(&skeletonFrame, NULL);

	/* Look if Kinect is currently tracking any skeletons. 6 is maximum number of skeletons Kinect will track. Take first available and stop. */
	for (i = 0; i < 6; i++){
		if (skeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED){
			this->determine_gesture(skeletonFrame.SkeletonData[i]);
			break;
		}
	}
}

/// <summary>
/// Main processing function
/// </summary>
void Gesture::Update()
{
	/* Clear any previous user_input and user_arg */
	user_input = NULL_CMD_MASK;
	user_arg = 0.0;

	if (NULL == m_pNuiSensor)
	{
		return;
	}

	/* Wait for new frame to become available from Kinect. Timeout: WAIT_FOR_FRAME_TIME_MS */
	if (WAIT_OBJECT_0 == WaitForSingleObject(m_hNextSkeletonEvent, WAIT_FOR_FRAME_TIME_MS))
	{
		ProcessSkeleton();
	}
}

void Gesture::determine_gesture(const NUI_SKELETON_DATA & skeleton)
{
	/* Extract desired skeleton data from argument struct */
	const Vector4& rh = skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT];
	const Vector4& lh = skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HAND_LEFT];
	const Vector4& rs = skeleton.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_RIGHT];
	const Vector4& re = skeleton.SkeletonPositions[NUI_SKELETON_POSITION_ELBOW_RIGHT];
	const Vector4& le = skeleton.SkeletonPositions[NUI_SKELETON_POSITION_ELBOW_LEFT];
	const Vector4& rhip = skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HIP_RIGHT];
	const Vector4& lhip = skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HIP_LEFT];

	/* Determine gesture and set user_input and user_arg as appropriate */
	// Following 2 if loops allow us to reset variables when that arm is not in use, prevents unexpected results
	if (rh.y < rhip.y) {
		mana = 0;
		manb = 0;
		stop = 0;
		turncount = 0;
		result = 0;
	}
	if (lh.y < lhip.y) {
		autoa = 0;
		autob = 0;
		turncount = 0;
		result = 0;
	}
	// When hands are both above hips, recognized as a turn, returns angle measurement based on hand placement every frame
	if (rh.y > rhip.y && lh.y > lhip.y && rh.y != lh.y) {
		backwarda = 0;
		backwardb = 0;
		forwarda = 0;
		forwardb = 0;
		autoa = 0;
		autob = 0;
		mana = 0;
		manb = 0;
		stop = 0;
		turncount++;
		if (rh.x == lh.x) {
			if (lh.y > rh.y) {
				result -= PI / 2;
			}
			else {
				result += PI / 2;
			}
		}
		else if (rh.x>lh.x) {
			result += atan((rh.y - lh.y) / (rh.x - lh.x));
		}
		if (turncount == 1) {
			user_arg = result / 1;
			if(user_arg < 0) user_input |= TURN_R_CMD_MASK;
			else user_input |= TURN_L_CMD_MASK;
			clearall();
		}
	}
	// Used to prevent turning from occuring continuously
	else user_input |= STOP_TURN_CMD_MASK;
	// When the user's right hand is 0.15m above the right shoulder for 5 frames, return the stop command
	if (rh.y > (rs.y + 0.15)) {
		backwarda = 0;
		backwardb = 0;
		if (stop == 5) {
			clearall();
			user_input |= STOP_CMD_MASK;
		}
		else if (stop <= 4) {
			stop++;
		}
	}
	// After 2 motions with the right hand calling the robot towards you, returns the forward command
	if (rh.y > re.y && forwarda > 0) {
		backwarda = 0;
		backwardb = 0;
		if (forwarda == 2) {
			user_input |= FORWARD_CMD_MASK;
			clearall();
		}
		else {
			forwardb = 1;
		}
	}
	else if (rh.y < re.y) {
		if (forwardb == 1) {
			forwarda = 2;
		}
		else {
			forwarda = 1;
		}
	}
	// After 2 motions with the left hand sending the robot away, returns the backward command
	if (lh.y < le.y && backwarda > 0) {
		forwarda = 0;
		forwardb = 0;
		if (backwarda == 2) {
			user_input |= REVERSE_CMD_MASK;
			clearall();
		}
		else {
			backwardb = 1;
		}
	}
	else if (lh.y > le.y) {
		if (backwardb == 1) {
			backwarda = 2;
		}
		else {
			backwarda = 1;
		}
	}
	// After 2 waves of the left hand, returns the auto command
	if (lh.x > le.x && autoa > 0 && lh.y > le.y) {
		mana = 0;
		manb = 0;
		if (autoa == 2) {
			user_input |= AUTO_MODE_CMD_MASK;
			clearall();
		}
		else {
			autob = 1;
		}
	}
	else if (lh.x < le.x && lh.y > le.y) {
		if (autob == 1) {
			autoa = 2;
		}
		else {
			autoa = 1;
		}
	}
	// After 2 waves of the right hand, returns the manual command
	if (rh.x < re.x && mana > 0 && rh.y > re.y) {
		autoa = 0;
		autob = 0;
		if (mana == 2) {
			user_input |= MANUAL_MODE_CMD_MASK;
			clearall();
		}
		else {
			manb = 1;
		}
	}
	else if (rh.x > re.x && rh.y > re.y) {
		if (manb == 1) {
			mana = 2;
		}
		else {
			mana = 1;
		}
	}
}

/// <summary>
/// Create the first connected Kinect found 
/// </summary>
/// <returns>indicates success or failure</returns>
HRESULT Gesture::CreateFirstConnected()
{
	INuiSensor * pNuiSensor;

	int iSensorCount = 0;
	HRESULT hr = NuiGetSensorCount(&iSensorCount);
	if (FAILED(hr))
	{
		return hr;
	}

	// Look at each Kinect sensor
	for (int i = 0; i < iSensorCount; ++i)
	{
		// Create the sensor so we can check status, if we can't create it, move on to the next
		hr = NuiCreateSensorByIndex(i, &pNuiSensor);
		if (FAILED(hr))
		{
			continue;
		}

		// Get the status of the sensor, and if connected, then we can initialize it
		hr = pNuiSensor->NuiStatus();
		if (S_OK == hr)
		{
			m_pNuiSensor = pNuiSensor;
			break;
		}

		// This sensor wasn't OK, so release it since we're not using it
		pNuiSensor->Release();
	}

	if (NULL != m_pNuiSensor)
	{
		// Initialize the Kinect and specify that we'll be using skeleton
		hr = m_pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_SKELETON);
		if (SUCCEEDED(hr))
		{
			// Create an event that will be signaled when skeleton data is available
			m_hNextSkeletonEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

			// Open a skeleton stream to receive skeleton data
			hr = m_pNuiSensor->NuiSkeletonTrackingEnable(m_hNextSkeletonEvent, 0);
		}
	}

	if (NULL == m_pNuiSensor || FAILED(hr))
	{
		printf("No ready Kinect found!\n");
		return E_FAIL;
	}

	return hr;
}

void Gesture::clearall() {
	stop = 0;
	forwarda = 0;
	forwardb = 0;
	backwarda = 0;
	backwardb = 0;
	autoa = 0;
	autob = 0;
	mana = 0;
	manb = 0;
	result = 0;
	turncount = 0;
}
