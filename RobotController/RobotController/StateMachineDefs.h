#pragma once

/* Machine inputs bit masks */
#define NULL_CMD_MASK			0x0000
#define STOP_CMD_MASK			0x0001
#define FORWARD_CMD_MASK		0x0002
#define REVERSE_CMD_MASK		0x0004
#define TURN_L_CMD_MASK			0x0008
#define TURN_R_CMD_MASK			0x0010
#define STOP_TURN_CMD_MASK		0x0020
#define MANUAL_MODE_CMD_MASK	0x0040
#define AUTO_MODE_CMD_MASK		0x0080
#define WALL_SENSOR_MASK		0x0100
#define LEFT_SENSOR_MASK		0x0200
#define RIGHT_SENSOR_MASK		0x0400