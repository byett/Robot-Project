/*
 * Copyright (C) 2012 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <gazebo/transport/transport.hh>
#include <gazebo/msgs/msgs.hh>
#include <gazebo/gazebo_client.hh>
//#include <gazebo/math/gzmath.hh>
#include <gazebo/math/Pose.hh>
#include <ignition/math/Pose3.hh>

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>


#define TCP_PORT "18424"
#define UDP_PORT "18423"
#define BACKLOG  10
#define TURN_ARG_SCALE_FACTOR   1.0 /* Factor for scaling and giving proper sign to turn angle argument received from RobotController */

/* Gazebo sensor ID's */
#define	GAZEBO_SENSOR_COUNT		5
#define GAZEBO_SENSOR_BASE		0xA0
#define WALL_ID				0xA0
#define LEFT_ID				0xA1
#define LEFTFRONT_ID			0xA2
#define RIGHT_ID			0xA3
#define RIGHTFRONT_ID			0xA4

/* Gazebo/User command ID's */
#define NULL_CMD			0x00
#define STOP_CMD			0xB1
#define FORWARD_CMD			0xB2
#define REVERSE_CMD			0xB3
#define TURN_L_CMD			0xB4
#define TURN_R_CMD			0xB5
#define FORWARD_L_CMD	        0xB6
#define FORWARD_R_CMD		0xB7
#define REVERSE_L_CMD		0xB8
#define REVERSE_R_CMD		0xB9
#define MANUAL_MODE_CMD	  	0xBA
#define AUTO_MODE_CMD		0xBB

// Global Socket ID. Bad idea, for testing only.
int tcp_socket, udp_socket;
struct addrinfo *servinfo;
char ip_address[INET6_ADDRSTRLEN];

/////////////////////////////////////////////////////
// Networking helper functions
// Copied from "Beej's Networking Guide"

// get sockaddr, IPv4 or IPv6:
void* get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int connect_to_server_tcp()
{
  // Open Communication Socket and wait for connection
  int sockfd;  // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *p;
  socklen_t sin_size;
  int rv;
  char s[INET6_ADDRSTRLEN];

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(ip_address, TCP_PORT, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return -1;
  }

  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1){
   	 close(sockfd);
   	 perror("connect_to_server: connect");
   	 continue;
    }
    break;
  }
  
  if (p == NULL){
    std::cout << "server: failed to bind" << std::endl;
    return -1;;
  }

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
  printf("Connected TCP Socket to address %s\n", s);

  freeaddrinfo(servinfo);

  tcp_socket = sockfd;
  return 0;
}

int connect_to_server_udp()
{
  // Open Communication Socket and wait for connection
  int sockfd;  // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *p;
  socklen_t sin_size;
  int rv;
  char s[INET6_ADDRSTRLEN], init_msg[8];
  struct sockaddr_storage their_addr;
  socklen_t addr_len;

  memset(init_msg, 1, sizeof init_msg);
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;

  if ((rv = getaddrinfo(ip_address, UDP_PORT, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return -1;
  }

  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }
    break;
  }

  if (p == NULL){
    std::cout << "server: failed to bind" << std::endl;
    return -1;;
  }
  servinfo = p;

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
  printf("Opened UDP Socket to address %s.\n", s);

  while(1){
    sleep(1);
    printf("Sending handshake message...\n");
    sendto(sockfd, init_msg, sizeof(init_msg), 0, servinfo->ai_addr, servinfo->ai_addrlen);

    addr_len = sizeof(struct sockaddr);
    memset(init_msg, 0, sizeof init_msg);
    rv = recvfrom(sockfd, init_msg, sizeof(init_msg), MSG_DONTWAIT, (struct sockaddr*)&their_addr, &addr_len);
    if( rv == -1 ){
      if( (errno == EWOULDBLOCK) || (errno == EAGAIN) )
	continue;
      else{
	printf("UDP handshake error.\n");
	return -1;
      }
    }

    else{
      printf("UDP received handshake.\n");
      break;
    }
  }

  udp_socket = sockfd;
  return 0;
}

int recvCmd(int *id, double *arg)
{
  int status;
  struct sockaddr_storage their_addr;
  socklen_t addr_len;
  char buf[12];

  addr_len = sizeof(struct sockaddr);
  memset(&buf, 0, sizeof(buf));
  status = recv(tcp_socket, (void*)buf, sizeof(buf), 0);

  if(status == -1){
    if( errno == EAGAIN || errno == EWOULDBLOCK ){
      *id = -1;
      *arg = -1.0;
    }
    else
      std::cout << "ERROR: Receiving command failed. errno:" << errno << std::endl;
  }
  else{
    if(status != 12) std::cout << "Warning: Recieved message with unexpected size." << std::endl;
    memcpy(id, &buf[0], sizeof(int));
    memcpy(arg, &buf[sizeof(int)], sizeof(double));
  }

  return status;
}


/////////////////////////////////////////////////
// Callback definitions.
// This could be done with fewer functions, but Gazebo doesn't support additional arguments
// and Implementing as a class would be more time consuming
void cb_send(void* msg_buf, int msg_size)
{
  int status;
  status = sendto(udp_socket, msg_buf, msg_size, 0, servinfo->ai_addr, servinfo->ai_addrlen);
  if(status == -1) std::cout << "Send Error. errno: " << errno << std::endl;
}

void cb_wall(ConstLaserScanStampedPtr &_msg)
{
  int id, size;
  double range;
  range = ((gazebo::msgs::LaserScan)(_msg->scan())).ranges(0);
  id = (int)WALL_ID;
  size = sizeof(range) + sizeof(id);

  uint8_t buf[size];
  memcpy( &buf[0], &id, sizeof(id) );
  memcpy( &buf[sizeof(id)], &range, sizeof(range) );

  cb_send( buf, size );
}

void cb_left(ConstLaserScanStampedPtr &_msg)
{
  int id, size;
  double range;
  range = ((gazebo::msgs::LaserScan)(_msg->scan())).ranges(0);
  id = (int)LEFT_ID;
  size = sizeof(range) + sizeof(id);

  uint8_t buf[size];
  memcpy( &buf[0], &id, sizeof(id) );
  memcpy( &buf[sizeof(id)], &range, sizeof(range) );

  cb_send( buf, size );
}

void cb_leftfront(ConstLaserScanStampedPtr &_msg)
{
  int id, size;
  double range;
  range = ((gazebo::msgs::LaserScan)(_msg->scan())).ranges(0);
  id = (int)LEFTFRONT_ID;
  size = sizeof(range) + sizeof(id);

  uint8_t buf[size];
  memcpy( &buf[0], &id, sizeof(id) );
  memcpy( &buf[sizeof(id)], &range, sizeof(range) );

  cb_send( buf, size );
}

void cb_right(ConstLaserScanStampedPtr &_msg)
{
  int id, size;
  double range;
  range = ((gazebo::msgs::LaserScan)(_msg->scan())).ranges(0);
  id = (int)RIGHT_ID;
  size = sizeof(range) + sizeof(id);

  uint8_t buf[size];
  memcpy( &buf[0], &id, sizeof(id) );
  memcpy( &buf[sizeof(id)], &range, sizeof(range) );

  cb_send( buf, size );
}

void cb_rightfront(ConstLaserScanStampedPtr &_msg)
{
  int id, size;
  double range;
  range = ((gazebo::msgs::LaserScan)(_msg->scan())).ranges(0);
  id = (int)RIGHTFRONT_ID;
  size = sizeof(range) + sizeof(id);

  uint8_t buf[size];
  memcpy( &buf[0], &id, sizeof(id) );
  memcpy( &buf[sizeof(id)], &range, sizeof(range) );

  cb_send( buf, size );
}

/////////////////////////////////////////////////
int main(int _argc, char **_argv)
{
  // Load gazebo
  std::cout << "Loading gazebo client...";
  std::cout.flush();
  gazebo::client::setup(_argc, _argv);
  std::cout << "done." << std::endl;

  // Create our node for communication
  std::cout << "Initializing gazebo communication node...";
  std::cout.flush();
  gazebo::transport::NodePtr node(new gazebo::transport::Node());
  node->Init();
  std::cout << "done." << std::endl;

  // Construct publisher and wait for listener
  std::cout << "Initializing velocity command publisher...";
  std::cout.flush();
  gazebo::transport::PublisherPtr velCmdPub = node->Advertise<gazebo::msgs::Pose>("~/create/vel_cmd");
  std::cout << "done." << std::endl;
  std::cout << "Waiting for command subscriber...";
  std::cout.flush();
  velCmdPub->WaitForConnection();
  std::cout << "connected." << std::endl;

  // Connect to Server, TCP then UDP. BLOCKING.
  std::cout << "Enter IP Address to connect to: ";
  std::cin.getline(ip_address, INET6_ADDRSTRLEN);
  std::cout << "Attempting TCP connection..." << std::endl;
  connect_to_server_tcp();
  std::cout << "Attempting UDP connection..." << std::endl;
  connect_to_server_udp();
  
  // Subscribe to Gazebo topics
  std::cout << "Starting topic subscribers...";
  std::cout.flush();
  gazebo::transport::SubscriberPtr wall_sensor_sub, left_sensor_sub, leftfront_sensor_sub, right_sensor_sub, rightfront_sensor_sub;
  std::string base_path = "~/create/base/";
  wall_sensor_sub       = node->Subscribe( base_path + "wall_sensor/scan", cb_wall);
  left_sensor_sub       = node->Subscribe( base_path + "left_cliff_sensor/scan", cb_left);
  leftfront_sensor_sub  = node->Subscribe( base_path + "leftfront_cliff_sensor/scan", cb_leftfront);
  right_sensor_sub      = node->Subscribe( base_path + "right_cliff_sensor/scan", cb_right);
  rightfront_sensor_sub = node->Subscribe( base_path + "rightfront_cliff_sensor/scan", cb_rightfront);
  std::cout << "done." << std::endl;

  // Messages
  ignition::math::Pose3d forward(1,0,0,0,0,0);
  ignition::math::Pose3d reverse(-1,0,0,0,0,0);
  ignition::math::Pose3d stop(0,0,0,0,0,0);
  ignition::math::Pose3d forwardL(1,0,0,0,0,3.0);
  ignition::math::Pose3d reverseL(-1,0,0,0,0,3.0);
  ignition::math::Pose3d stopL(0,0,0,0,0,3.0);
  ignition::math::Pose3d turn;
  gazebo::msgs::Pose msg;
  int cmd_id, round_arg;
  bool timed_cmd_executing;
  double cmd_arg;
  char buf[sizeof(int) + sizeof(double)];
  struct timeval stopTime, curTime;

  timed_cmd_executing = false;

  std::cout << "Starting main program loop." << std::endl;
  while (true){
    recvCmd(&cmd_id, &cmd_arg);
    gettimeofday(&curTime, NULL);
    
    if(cmd_id > 0){
      switch(cmd_id){
      case FORWARD_CMD:
        msg = gazebo::msgs::Convert(forward);
        //gazebo::msgs::Set(&msg, forward);
        //msg->Set(1.0, 0.0, 0.0, 0.0, 0.0, 0.0);
        break;
      case REVERSE_CMD:
        msg = gazebo::msgs::Convert(reverse);
        //gazebo::msgs::Set(&msg, reverse);
        //msg->Set(-1.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    break;
      case STOP_CMD:
        msg = gazebo::msgs::Convert(stop);
        //gazebo::msgs::Set(&msg, stop);
        //msg->Set(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
        break;
      case TURN_L_CMD:
      case TURN_R_CMD:
        //gazebo::msgs::Set(&msg, ignition::math::Pose3d(ignition::math::Vector3d(0,0,0), ignition::math::Quaterniond(0,0,TURN_ARG_SCALE_FACTOR*cmd_arg)));
        msg = gazebo::msgs::Convert(ignition::math::Pose3d(ignition::math::Vector3d(0,0,0), ignition::math::Quaterniond(0,0,TURN_ARG_SCALE_FACTOR*cmd_arg)));
        //gazebo::msgs::Set(&msg, turn);
        //msg->Set(0.0, 0.0, 0.0, 0.0, 0.0, TURN_ARG_SCALE_FACTOR*cmd_arg);
        break;
      case FORWARD_L_CMD:
      case FORWARD_R_CMD:
        turn.Set(1,0,0,0,0,TURN_ARG_SCALE_FACTOR*cmd_arg);
        msg = gazebo::msgs::Convert(forwardL);
        //gazebo::msgs::Set(&msg, turn);
        //msg->Set(1.0, 0.0, 0.0, 0.0, 0.0, TURN_ARG_SCALE_FACTOR*cmd_arg);
	break;
      case REVERSE_L_CMD:
      case REVERSE_R_CMD:
        turn.Set(-1,0,0,0,0,TURN_ARG_SCALE_FACTOR*cmd_arg);
        msg = gazebo::msgs::Convert(forwardL);
        //gazebo::msgs::Set(&msg, turn);
        //msg->Set(-1.0, 0.0, 0.0, 0.0, 0.0, TURN_ARG_SCALE_FACTOR*cmd_arg);
	break;
      default:
	std::cout << "Unknown command ID: " << cmd_id << std::endl;
	break;
      } /* End switch */
      velCmdPub->Publish( msg );
    } /* End if */


    /* Code for executing commands for a certain amount of time and sending a SUCCESS message  
      round_arg = (int)round( cmd_arg );
      stopTime = curTime;
      if(round_arg > 0){
	stopTime.tv_sec += round_arg / 1000;
	stopTime.tv_usec += (round_arg % 1000) * 1000;
	stopTime.tv_sec += stopTime.tv_usec / 1000000;
	stopTime.tv_usec = (stopTime.tv_usec % 1000000);
	timed_cmd_executing = true;
      }
      else timed_cmd_executing = false;
    }
    
    if( timed_cmd_executing ){
      if( (stopTime.tv_sec < curTime.tv_sec) ||
	  ((stopTime.tv_sec == curTime.tv_sec) && (stopTime.tv_usec < curTime.tv_usec)) ){
	cmd_id = CMD_SUCCESS;
	cmd_arg = 0.0;
	memcpy( &buf[0], &cmd_id, sizeof(cmd_id) );
	memcpy( &buf[sizeof(cmd_id)], &cmd_arg, sizeof(cmd_arg) );
	cb_send( buf, sizeof(cmd_id) + sizeof(cmd_arg) );
	gazebo::msgs::Set(&msg, stop);
	velCmdPub->Publish( msg );
	timed_cmd_executing = false;
      }
    }
    */

    gazebo::common::Time::MSleep(1);
  } /* End while */

  /* Make sure to shut everything down. */
  close(tcp_socket);
  close(udp_socket);
  freeaddrinfo(servinfo);
  gazebo::client::shutdown();
}


