#ifndef COMMANDLINE_CLIENTFUNCTIONALITIES_H_
#define COMMANDLINE_CLIENTFUNCTIONALITIES_H_

#define LOG_DIR "../logs/"

#include <chrono>
#include <ctime>
#include <iostream>
#include <map>
#include <memory>
#include <ratio>
#include <string>
#include <thread>

#include <glog/logging.h>
#include <grpcpp/client_context.h>
#include <grpcpp/grpcpp.h>
#include "../chirp.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using chirp::ChirpReply;
using chirp::ChirpRequest;
using chirp::FollowReply;
using chirp::FollowRequest;
using chirp::KeyValueStore;
using chirp::MonitorReply;
using chirp::MonitorRequest;
using chirp::ReadReply;
using chirp::ReadRequest;
using chirp::RegisterReply;
using chirp::RegisterRequest;
using chirp::ServiceLayer;
/**
  ClientFunctionalities is a class that the Command Line Client will call when
  users are registering, sending a chirp, following other chirpers, and etc.
*/
class ClientFunctionalities {
 public:
  /*
    Constructor take takes in a shared pointer of Channel
  */
  explicit ClientFunctionalities(const std::shared_ptr<Channel> &channel)
      : stub_(chirp::ServiceLayer::NewStub(channel)) {}
  /*
    Create a new user with username
  */
  void registeruser(const std::string &username);
  /*
    Add a chirp to this username
  */
  void chirp(const std::string &username, const std::string &text,
             const std::string &parent_id);
  /*
    Follow a chirper with this username
  */
  void follow(const std::string &username, const std::string &to_follow);
  /*
    Read all the chirps from this user
  */
  void read(const std::string &chirp_id);
  /*
    Read all the chirps that their followers chirps in real-time
  */
  void monitor(const std::string &username);
  /*
    Streams all new chirps containing ‘hashtag’
  */
  void stream(const std::string &hashtag);

 private:
  /*
    Unique ptr for all the stub
  */
  std::unique_ptr<chirp::ServiceLayer::Stub> stub_;
};
#endif /* CLIENT_FUNCTIONALITIES_H_*/
