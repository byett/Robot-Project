#pragma once

/* Gazebo sensor ID's */
#define	GAZEBO_SENSOR_COUNT		5
#define GAZEBO_SENSOR_BASE		0xA0
#define WALL_ID					0xA0
#define LEFT_ID					0xA1
#define LEFTFRONT_ID			0xA2
#define RIGHT_ID				0xA3
#define RIGHTFRONT_ID			0xA4

/* Gazebo Command ID's */
#define STOP_CMD		0xB0
#define FORWARD_CMD		0xB1
#define REVERSE_CMD		0xB2
#define TURN_CMD		0xB3

#define GAZEBO_DATA_MSG_SIZE	sizeof(int) + sizeof(double)
#define	GAZEBO_CMD_MSG_SIZE		sizeof(int) + sizeof(double)

typedef struct {
	double	sensor_ranges[GAZEBO_SENSOR_COUNT];
}gazeboSensorData;