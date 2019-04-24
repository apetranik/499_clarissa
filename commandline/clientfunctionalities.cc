#include "clientfunctionalities.h"

void ClientFunctionalities::registeruser(const std::string &username) {
  if (username == "") {
    return;
  }
  chirp::RegisterRequest request;
  request.set_username(username);
  chirp::RegisterReply reply;

  ClientContext context;
  Status status = stub_->registeruser(&context, request, &reply);

  if (status.ok()) {
    std::cout << "Registered user: " << username << std::endl;
  } else {
    std::cout << status.error_code() << ": "
              << "Register user unsuccessful: " << username << std::endl;
  }
}
void ClientFunctionalities::chirp(const std::string &username,
                                  const std::string &text,
                                  const std::string &parent_id) {
  chirp::ChirpRequest request;
  request.set_username(username);
  request.set_text(text);
  request.set_parent_id(parent_id);

  chirp::ChirpReply reply;
  ClientContext context;
  Status status = stub_->chirp(&context, request, &reply);

  if (status.ok()) {
    std::cout << "Successfully chirped with id: " << reply.chirp().id()
              << std::endl;
  } else {
    std::cout << status.error_code() << ": "
              << "Chirp is unsuccessful. Try again." << std::endl;
  }
}
void ClientFunctionalities::follow(const std::string &username,
                                   const std::string &to_follow) {
  chirp::FollowRequest request;
  request.set_username(username);
  request.set_to_follow(to_follow);
  chirp::FollowReply reply;

  ClientContext context;
  Status status = stub_->follow(&context, request, &reply);

  if (status.ok()) {
    std::cout << "Sucessfully followed: " << to_follow << std::endl;
  } else {
    std::cout << status.error_code() << ": "
              << "Following " << to_follow << " is unsuccessful" << std::endl;
  }
}
void ClientFunctionalities::read(const std::string &chirp_id) {
  /*
    Tested: Able to recieve replied chirps from chirp_id!
  */
  chirp::ReadRequest request;
  request.set_chirp_id(chirp_id);
  chirp::ReadReply reply;
  ClientContext context;
  Status status = stub_->read(&context, request, &reply);

  std::multimap<std::string, std::string> mymap;

  for (int i = 0; i < reply.chirps_size(); i++) {
    chirp::Chirp current_chirp = reply.chirps(i);
    std::time_t seconds = current_chirp.timestamp().seconds();
    std::cout << "[" << current_chirp.username()
              << "]: " << current_chirp.text() << std::endl;
  }

  if (status.ok()) {
    std::cout << "Successfully read all chirps with thread chirp" << chirp_id
              << std::endl;
  } else {
    std::cout << status.error_code() << ": "
              << "Reading thread " << chirp_id << " is unsuccessful."
              << std::endl;
  }
}
void ClientFunctionalities::monitor(const std::string &username) {
  /*
  Tested: Able to recieve new chirps
*/
  chirp::MonitorRequest request;
  request.set_username(username);
  chirp::MonitorReply reply;
  ClientContext context;
  std::unique_ptr<grpc::ClientReader<chirp::MonitorReply>> reader(
      stub_->monitor(&context, request));
  while (true) {
    if (reader->Read(&reply)) {
      std::cout << "[" << reply.chirp().username()
                << "]: " << reply.chirp().text() << std::endl;
    }
    /* Polls for every half a second so that it doesn't have to keep polling too
     * many times */
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}
void ClientFunctionalities::stream(const std::string &username,
                                   const std::string &hashtag) {
  chirp::StreamRequest request;
  request.set_hashtag(hashtag);
  request.set_username(username);

  chirp::StreamReply reply;
  ClientContext context;
  std::unique_ptr<grpc::ClientReader<chirp::StreamReply>> reader(
      stub_->stream(&context, request));
  while (true) {
    if (reader->Read(&reply)) {
      for (int i = 0; i < reply.chirps_size(); i++) {
        std::cout << "[" << reply.chirps(i).username()
                  << "]: " << reply.chirps(i).text() << std::endl;
      }
    }
    /* Polls for every half a second so that it doesn't have to keep polling too
     * many times */
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}
