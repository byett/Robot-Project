/*****************************************************
*	Gesture.cpp
*
*	Kinect Gesture Recognition
*
*	Author: Charles Hartsell
*			Ben Yett
*	Date:	4-3-17
*****************************************************/

#include "stdafx.h"

#include "Gesture.h"
#include <strsafe.h>
#include "resource.h"
#include <math.h>
#include <windows.h>
#include <NuiApi.h>
#include <NuiSkeleton.h>
#include <iostream>    
#include <stdlib.h>
#include <string>
#include <fstream>

#define PI 3.14159265

Gesture::Gesture() :
	m_pNuiSensor(NULL),
	m_hNextSkeletonEvent(INVALID_HANDLE_VALUE),
	m_pSkeletonStreamHandle(INVALID_HANDLE_VALUE)
{
	clearall();
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

void Gesture::ProcessSkeleton()
{
	NUI_SKELETON_FRAME skeletonFrame = { 0 };

	HRESULT hr = m_pNuiSensor->NuiSkeletonGetNextFrame(0, &skeletonFrame);
	if (FAILED(hr))
	{
		//printf("ProcessSkeleton failed.\n");
		return;
	}
	printf("ProcessSkeleton succeded.\n");

	// smooth out the skeleton data
	m_pNuiSensor->NuiTransformSmooth(&skeletonFrame, NULL);

	this->determine_gesture(skeletonFrame.SkeletonData[0]);
}

/// <summary>
/// Main processing function
/// </summary>
void Gesture::Update()
{
	if (NULL == m_pNuiSensor)
	{
		return;
	}

	// Wait for 0ms, just quickly test if it is time to process a skeleton
	if (WAIT_OBJECT_0 == WaitForSingleObject(m_hNextSkeletonEvent, 100))
	{
		ProcessSkeleton();
	}
}

void Gesture::determine_gesture(const NUI_SKELETON_DATA & skeleton)
{
	Sleep(20);
	std::ofstream myfile;
	myfile.open("commands.txt", std::ofstream::app);
	const Vector4& rh = skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT];
	const Vector4& lh = skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HAND_LEFT];
	const Vector4& rs = skeleton.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_RIGHT];
	const Vector4& re = skeleton.SkeletonPositions[NUI_SKELETON_POSITION_ELBOW_RIGHT];
	const Vector4& le = skeleton.SkeletonPositions[NUI_SKELETON_POSITION_ELBOW_LEFT];
	const Vector4& rhip = skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HIP_RIGHT];
	const Vector4& lhip = skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HIP_LEFT];
	//myfile << rh.x << "," << rh.y << ',' << lh.x << ',' << lh.y << ',' << rs.y << ',' << re.y << ',' << le.y << '\n';
	if (rh.y>(rhip.y + 0) && lh.y>(lhip.y + 0) && rh.y != lh.y) {
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
				result += 90;
				//myfile << "90\n";
			}
			else {
				result += -90;
				//myfile << "-90\n";
			}
		}
		else if (rh.x>lh.x) {
			result += atan((lh.y - rh.y) / (rh.x - lh.x)) * 180 / PI;
			//myfile << result << "\n";
		}
		if (turncount == 10) {
			result = result / 10;
			myfile << result << "\n";
			clearall();
		}
	}
	if (rh.y > (rs.y + 0.15)) {
		backwarda = 0;
		backwardb = 0;
		if (stop == 10) {
			clearall();
			myfile << "stop\n";
		}
		else if (stop <= 9) {
			stop++;
		}
	}
	if (rh.y > re.y && forwarda > 0) {
		backwarda = 0;
		backwardb = 0;
		if (forwarda == 2) {
			myfile << "forward\n";
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
	if (lh.y < le.y && backwarda > 0) {
		forwarda = 0;
		forwardb = 0;
		autoa = 0;
		autob = 0;
		if (backwarda == 2) {
			myfile << "backward\n";
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
	if (lh.x > le.x && autoa > 0 && lh.y>lhip.y) {
		mana = 0;
		manb = 0;
		if (autoa == 2) {
			myfile << "auto\n";
			clearall();
		}
		else {
			autob = 1;
		}
	}
	else if (lh.x < le.x && lh.y>lhip.y) {
		if (autob == 1) {
			autoa = 2;
		}
		else {
			autoa = 1;
		}
	}
	if (rh.x < re.x && mana > 0 && rh.y>rhip.y) {
		autoa = 0;
		autob = 0;
		if (mana == 2) {
			myfile << "manual\n";
			clearall();
		}
		else {
			manb = 1;
		}
	}
	else if (rh.x > re.x && rh.y>rhip.y) {
		if (manb == 1) {
			mana = 2;
		}
		else {
			mana = 1;
		}
	}
	/*if (lh.y < le.y && backward==1) {
	backward = 0;
	myfile << "backward\n";
	}
	else {
	backward = 0;
	}
	if (lh.y > le.y) {
	backward++;
	//Sleep(500);
	}*/
	myfile.close();
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