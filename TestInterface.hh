/*
 * Basic PLEXIL Interface Adapter
 * Intended for testing detailed semantics of PLEXIL interfacing
 * 
 * Charles Hartsell 1-18-2017
 */

#pragma once

#include <iostream>
#include <ctime>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include "Command.hh"
#include "InterfaceAdapter.hh"
#include "Value.hh"

using namespace PLEXIL;

class TestInterface : public InterfaceAdapter
{
public:
  TestInterface (AdapterExecInterface&, const pugi::xml_node&);
  // using compiler's default copy constructor, assignment, destructor
  
  virtual bool initialize();
  virtual bool start();
  virtual bool stop();
  virtual bool reset();
  virtual bool shutdown();
  virtual void invokeAbort(Command *cmd);

  virtual void executeCommand(Command *cmd);
  virtual void lookupNow (State const& state, StateCacheEntry &entry);
  virtual void subscribe(const State& state);
  virtual void unsubscribe(const State& state);
  void propagateValueChange (const State&, const std::vector<Value>&) const;

protected:
  int openSocket();
  int sendCmd(int id, double arg);
  static void *listen(void *arg);

  struct sockaddr_storage their_addr;
  socklen_t addr_len;
  pthread_t listen_thread;
  struct addrinfo *servinfo;
  int socket_fd;
  double wall_range, left_range, leftfront_range, right_range, rightfront_range;
  std::set<State> subscribedStates;
  Command *last_cmd;
};

extern "C" {
  void initTestInterface();
}
