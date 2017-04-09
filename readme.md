# Robot Project

This project allows a user to control a simulated iRobot Create (using Gazebo simulation environment)
through body gestures (primarily hand motions) which are sensed using a Microsoft Kinect sensor.

## RobotController

RobotController is a Visual Studio project which performs a number of functions including:

* Gesture recognition using Microsoft Kinect sensor and Kinect SDK. (Gesture class)
* Monitor sensor data stream from and send commands to Gazebo. (NetSocket class and RobotController.cpp)
* Determine proper robot response using a Finite State Machine with both user and sensor data as inputs. (StateMachine class)

This VS project was originally built with VS Community 2015, and has also been built in VS Professional 2013 with some configuration of build tools ("Platform Toolset" in VS) required.

Project requires .NETFramework v4.5.2 and Kinect SDK v1.8 (possibly compatable with v2.0 - this has not been tested).
Must add Kinect .h and .lib folders to include and library directories.
With default Kinect SDK installation path, these can be found at:
C:\Program Files\Microsoft SDKs\Kinect\v1.8\("inc" and "lib\amd64" and "lib\x86")

## Gazebo

gazebo subdirectory includes test_world.sdf model file with all necessary models defined.

Intended to be run in a Linux environment (tested with Debian Jessie VirtualBox).
Run with "gazebo test_world.sdf" at terminal line once Gazebo has been installed.

gazeboInterface provides UDP and TCP sockets for RobotController to communicate with Gazebo.
Can be built with ./buildInterface.sh and then run with ./runInterface.sh.
Building requires CMake, Make, and gcc.