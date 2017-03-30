/*
 * Basic PLEXIL Interface Adapter
 * Intended for testing detailed semantics of PLEXIL interfacing
 * 
 * Charles Hartsell 1-18-2017
 */

#include "TestInterface.hh"

// PLEXIL API
#include "AdapterConfiguration.hh"
#include "AdapterFactory.hh"
#include "AdapterExecInterface.hh"
#include "Debug.hh"
#include "Expression.hh"
#include "StateCacheEntry.hh"

#include <string>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <errno.h>

#define PORT "18463"
#define IP_ADDR "127.0.0.1"
#define MAXDATASIZE 100

// Lookup value ID's
#define WALL_ID 0xA0
#define LEFT_ID 0xA1
#define LEFTFRONT_ID 0xA2
#define RIGHT_ID 0xA3
#define RIGHTFRONT_ID 0xA4

// Command ID's
#define STOP_CMD 0xB0
#define FORWARD_CMD 0xB1
#define REVERSE_CMD 0xB2
#define TURN_CMD 0xB3
#define CMD_SUCCESS 0xBE
#define CMD_FAIL 0xBF

using std::cout;
using std::cerr;
using std::endl;
using std::map;
using std::string;
using std::vector;
using std::copy;

static TestInterface *Adapter;
static vector<Value> const EmptyArgs;

//////////////////////////////////////////////////////////////
// Helper Functions
//////////////////////////////////////////////////////////////
static State createState (const string& state_name, const vector<Value>& value)
{
  State state(state_name, value.size());
  if (value.size() > 0)
  {
    for(size_t i=0; i<value.size();i++)
    {
      state.setParameter(i, value[i]);
    }
  }
  return state;
}

// Networking Helper functions copied from "Beej's Networking Guide"
// get sockaddr, IPv4 or IPv6:
static void* get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


//////////////////////////////////////////////////////////////
// Class definitions
//////////////////////////////////////////////////////////////
TestInterface::TestInterface(AdapterExecInterface& execInterface, const pugi::xml_node& configXml)
  : InterfaceAdapter(execInterface, configXml)
{
  debugMsg("TestInterface", " created.");
}

bool TestInterface::initialize()
{
  // Required boilerplate
  g_configuration->defaultRegisterAdapter(this);

  // Variable initilizations
  Adapter = this;
  wall_range = HUGE_VAL;
  left_range = 0.0;
  leftfront_range = 0.0;
  right_range = 0.0;
  rightfront_range = 0.0;
  last_cmd = NULL;

  // UDP Socket and listener thread initilization 
  socket_fd = openSocket();
  pthread_create( &listen_thread, NULL, TestInterface::listen, this);

  // Init finished
  debugMsg("TestInterface", " initialized.");
  return true;
}

bool TestInterface::start()
{
  debugMsg("TestInterface", " started.");
  return true;
}

bool TestInterface::stop()
{
  debugMsg("TestInterface", " stopped.");
  return true;
}

bool TestInterface::reset()
{
  debugMsg("TestInterface", " reset.");
  return true;
}

bool TestInterface::shutdown()
{
  pthread_cancel( listen_thread );
  close( socket_fd );
  freeaddrinfo( servinfo );
  debugMsg("TestInterface", " shutdown.");
  return true;
}

void TestInterface::invokeAbort(Command *cmd)
{
  const string &name = cmd->getName();
  debugMsg("TestInterface:Command", "Aborting command: " << name);
  bool retval;

  if (name == "drive" || name == "reverse" || name == "turn"){
    sendCmd(STOP_CMD, 0.0);
    retval = true;
  }
  else{
    debugMsg("TestInterface:Command", "Abort failed. Unknown command: " << name);
    retval = false;
  }

  cmd->acknowledgeAbort(retval);
}

void TestInterface::executeCommand(Command *cmd)
{
  const string &name = cmd->getName();
  debugMsg("TestInterface:Command","executing command: " << name);

  std::vector<Value> args;
  args = cmd->getArgValues();

  double d;
  string s;
  State st;
  
  if (name == "debugMsg"){
    args[0].getValue(s);
    debugMsg("TestInterface:Command", s);
  }
  else if (name == "drive"){
    args[0].getValue(d);
    sendCmd(FORWARD_CMD, d);
  }
  else if (name == "reverse"){
    args[0].getValue(d);
    sendCmd(REVERSE_CMD, d);
  }
  else if (name == "turn"){
    args[0].getValue(d);
    sendCmd(TURN_CMD, d);
  }
  else if (name == "dock"){
    ////////////// Need to handle command
  }
  else{
    debugMsg("TestInterface:Command", "Command failed. Unknown command: " << name);
  }

  // PLEXIL pends on command acknowledgement (does not block - concurrent portions can continue)
  last_cmd = cmd;
  m_execInterface.handleCommandAck(cmd, COMMAND_SENT_TO_SYSTEM);
}

void TestInterface::lookupNow(State const &state, StateCacheEntry &entry)
{
  string const &name = state.name();
  debugMsg("TestInterface:Lookup", "Received Lookup request for " << name);
  const vector<Value> &args = state.parameters();
  Value retval;

  if (name == "wall_sensor"){
    retval = wall_range;
  }
  else if (name == "left_sensor"){
    retval = left_range;
  }
  else if (name == "leftfront_sensor"){
    retval = leftfront_range;
  }
  else if (name == "right_sensor"){
    retval = right_range;
  }
  else if (name == "rightfront_sensor"){
    retval = rightfront_range;
  }
  else{
    debugMsg("TestInterface:Lookup", "LookupNow Failed. Unknown lookup variable: " << name);
    entry.setUnknown();
    return;
  }

  // Return result to PLEXIL
  entry.update(retval);
}

void TestInterface::subscribe(const State &state)
{
  debugMsg("TestInterface:subscribe", " subscribing to state: " << state.name());
  subscribedStates.insert(state);
}

void TestInterface::unsubscribe(const State &state)
{
  debugMsg("TestInterface:unsubscribe", " unsubscribing from state: " << state.name());
  subscribedStates.erase(state);
}

void TestInterface::propagateValueChange(const State &state, const vector<Value> &value) const
{
  m_execInterface.handleValueChange(state, value.front());
  m_execInterface.notifyOfExternalEvent();
}

int TestInterface::openSocket()
{
  // Taken from "Beej's Networking Guide"
  int sockfd, optval;  
  struct addrinfo hints, *p;
  int rv, init_msg;
  char s[INET6_ADDRSTRLEN];

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;
  optval = 1;

  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and connect to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
			 p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("client: bind");
      continue;
    }
    break;
  }

  if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    return 2;
  }

  // Wait to receive initial message and store server address info
  recvfrom(socket_fd, &init_msg, sizeof(int), 0, (struct sockaddr *)&their_addr, &addr_len);
  
  servinfo = p;
  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
  debugMsg("TestInterface:Socket", "UDP Socket initialized with address: " << s);
  
  return sockfd;
}

int TestInterface::sendCmd(int id, double arg)
{
  int status;
  char buf[sizeof(id) + sizeof(arg)];

  memcpy(&buf[0], &id, sizeof(id));
  memcpy(&buf[sizeof(id)], &arg, sizeof(arg));
  status = sendto(socket_fd, (void*)buf, sizeof(buf), 0, (struct sockaddr*)&their_addr, addr_len);
  if(status == -1) debugMsg("TestInterface:Socket", "Send Error: " << errno);

  return status;
}

void* TestInterface::listen(void *arg)
{
  TestInterface *context = (TestInterface*)arg;
  char buffer[ MAXDATASIZE ];
  string name;
  Value retval;
  State st;
  int id, size_id, size_range;
  double range;
  
  size_id = sizeof(id);
  size_range = sizeof(range);

  // Recieve data over socket
  while(true){
    recvfrom(context->socket_fd, buffer, MAXDATASIZE, 0,
	     (struct sockaddr *)&(context->their_addr), &(context->addr_len));
    memcpy( &id, &buffer[0], size_id );
    memcpy( &range, &buffer[size_id], size_range );

    if( id == CMD_SUCCESS ){
      debugMsg("TestInterface:Socket", "Received cmd id: " << id << " Sending COMMAND_SUCCESS");
      context->m_execInterface.handleCommandAck(context->last_cmd, COMMAND_SUCCESS);
    }
    else if( id == CMD_FAIL ){
      debugMsg("TestInterface:Socket", "Received cmd id: " << id << " Sending COMMAND_FAILED");
      context->m_execInterface.handleCommandAck(context->last_cmd, COMMAND_FAILED);
    }
    else{
      // check ID and set appropriate name
      switch(id){
      case WALL_ID:
	name = "wall_sensor";
	context->wall_range = range;
	break;
      case LEFT_ID:
	name = "left_sensor";
	context->left_range = range;
	break;
      case LEFTFRONT_ID:
	name = "leftfront_sensor";
	context->leftfront_range = range;
	break;
      case RIGHT_ID:
	name = "right_sensor";
	context->right_range = range;
	break;
      case RIGHTFRONT_ID:
	name = "rightfront_sensor";
	context->rightfront_range = range;
	break;
      default:
	debugMsg("TestInterface:Socket", "Recieved unknown sensor ID: " << id);
	break;
      }
      
      // Propigate change to PLEXIL
      retval = (Value)range;
      st = createState(name, EmptyArgs);
      context->propagateValueChange(st, vector<Value> (1, retval));
    }
  }

  return NULL;
}

// Required function
extern "C" {
  void initTestInterface() {
    REGISTER_ADAPTER(TestInterface, "TestInterface");
  }
}
