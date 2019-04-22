#include <unistd.h>
#include <thread>

#define STRIP_FLAG_HELP 1
#include <gflags/gflags.h>

#include "clientfunctionalities.h"

DEFINE_string(user, "", "username of user using chrip");
DEFINE_string(register, "", "register a new user with a username");
DEFINE_string(chirp, "", "text of a new chirp from user");
DEFINE_string(reply, "", "replies a chirp with a new chirp");
DEFINE_string(follow, "", "follows another user");
DEFINE_string(read, "", "reads the chirp thread starting at the given id");
DEFINE_bool(monitor, false, "streams new chirps from those currently followed");
DEFINE_string(stream, "", "Streams all new chirps containing ‘hashtag’");

static bool ValidateUser(const char *flagname, const std::string &value);
static bool ValidateChirp(const char *flagname, const std::string &value);
static bool ValidateReply(const char *flagname, const std::string &value);

DEFINE_validator(user, &ValidateUser);
DEFINE_validator(reply, &ValidateReply);

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  ClientFunctionalities client(grpc::CreateChannel(
      "localhost:50002", grpc::InsecureChannelCredentials()));
  if (FLAGS_register != "" && FLAGS_user == "" && FLAGS_chirp == "" &&
      FLAGS_reply == "" && FLAGS_follow == "" && FLAGS_read == "") {
    client.registeruser(FLAGS_register);
  } else if (FLAGS_user != "") {
    if (FLAGS_chirp == "" && FLAGS_reply == "" && FLAGS_follow == "" &&
        FLAGS_read == "" && !FLAGS_monitor) {
      std::cout << "No Commands. Try Again." << std::endl;
    } else if (FLAGS_chirp != "" && FLAGS_reply == "") {
      client.chirp(FLAGS_user, FLAGS_chirp, "");
    } else if (FLAGS_chirp != "" && FLAGS_reply != "") {
      client.chirp(FLAGS_user, FLAGS_chirp, FLAGS_reply);
    }
    if (FLAGS_follow != "") {
      client.follow(FLAGS_user, FLAGS_follow);
    }
    if (FLAGS_read != "") {
      client.read(FLAGS_read);
    }
    if (FLAGS_monitor) {
      client.monitor(FLAGS_user);
    }
    if (FLAGS_stream != "") {
      client.stream(FLAGS_stream);
    }
  } else {
    std::cout << "Syntax Error. Please try again." << std::endl;
  }
  return 0;
}

static bool ValidateUser(const char *flagname, const std::string &value) {
  if (FLAGS_register != "" || value != "") {
    return true;
  }
  return false;
}

static bool ValidateChirp(const char *flagname, const std::string &value) {
  if (value != "") {
    return true;
  }
  return false;
}
static bool ValidateReply(const char *flagname, const std::string &value) {
  if (value != "") {
    return ValidateChirp("chirp", FLAGS_chirp);
  }
  return true;
}
