#pragma once

/* Gazebo sensor ID's */
#define	GAZEBO_SENSOR_COUNT		5
#define GAZEBO_SENSOR_BASE		0xA0
#define WALL_ID					0xA0
#define LEFT_ID					0xA1
#define LEFTFRONT_ID			0xA2
#define RIGHT_ID				0xA3
#define RIGHTFRONT_ID			0xA4

/* Gazebo sensor ranges */
#define WALL_SENSOR_TRIP_RANGE		0.050
#define TILT_SENSOR_TRIP_RANGE		0.035

/* Gazebo/User command ID's */
#define NULL_CMD			0x00
#define STOP_CMD			0xB1
#define FORWARD_CMD			0xB2
#define REVERSE_CMD			0xB3
#define TURN_L_CMD			0xB4
#define TURN_R_CMD			0xB5
#define FORWARD_L_CMD		0xB6
#define FORWARD_R_CMD		0xB7
#define REVERSE_L_CMD		0xB8
#define REVERSE_R_CMD		0xB9
#define MANUAL_MODE_CMD		0xBA
#define AUTO_MODE_CMD		0xBB

/* Gazebo Message sizes*/
#define GAZEBO_DATA_MSG_SIZE	sizeof(int) + sizeof(double)
#define	GAZEBO_CMD_MSG_SIZE		sizeof(int) + sizeof(double)

typedef struct {
	double	sensor_ranges[GAZEBO_SENSOR_COUNT];
}gazeboSensorData;