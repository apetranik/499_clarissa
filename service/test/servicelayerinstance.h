#ifndef SERVICE_TEST_SERVICELAYERINSTANCE_H
#define SERVICE_TEST_SERVICELAYERINSTANCE_H

#include <queue>
#include <thread>

#include "../../chirp.grpc.pb.h"
#include "../../kvstore/test/keyvaluestoreinstance.h"

/*
  This class is to test the service layer functionalities. It does not
  go over GRPC. It uses KeyValueStoreInstance which is the tested verison of
  KeyValueStoreServer class that goes over gRPC.
*/
class ServiceLayerInstance {
 public:
  /* Constructor takes a KeyValueStoreInstance which stores the data */
  ServiceLayerInstance(KeyValueStoreInstance *kv_instance);
  /* Deconstructor deleted kvstore instantiation */
  ~ServiceLayerInstance();
  /* RegisterUser takes in a username to put into kvstore */
  bool RegisterUser(const std::string &username);
  /* Chirp takes in a username is associated to the chirp, the text, and
   * optional, a parent id */
  bool Chirp(const std::string &username, const std::string &text,
             const std::string &parentid);
  /* Follow takes in a username and another use that the username wants to
   * follow */
  bool Follow(const std::string &username, const std::string &tofollow);
  /* Read() takes in a chirpid and all the thread related to that chirp id */
  std::vector<chirp::Chirp> Read(const std::string &chirpid);
  /* Monitor takes in a username and display realtime chirps from the user's
   * followers */
  std::vector<chirp::Chirp> Monitor(const std::string &username);
  /* Stream takes in a hashtag and displays realtime chirps with that hashtag */
  std::vector<chirp::Chirp> Stream(const std::string &username,
                                   const std::string &hastag);
  /*
  The functions below are helper functions to serialize and parse objects
  this is made public so it can be tested in servicelayertest.cc
  */

  /*
  Converts serialized string to chirp::Chirp
  */
  chirp::Chirp ConvertToChirp(std::string byte);
  /*
  Converts serialized string to vector of chirp::Hashtag
  */
  std::vector<chirp::Chirp> ConvertToHashtag(const std::string byte);
  /*
  Converts serialized string to chirp::ChirpReplies (which is a list of
  replies to the same chirp<ID>)
  */
  chirp::ChirpReplies ConvertToChirpReplies(std::string byte);

  /*
    Copy chirps, "chirp_" is an empty chirp, and "reply_" is copying into
    "chirp"
  */
  void CopyChirp(chirp::Chirp *chirp_, const chirp::Chirp &reply_);
  /*
  Converts serialize string (byte) to chirp::User.
  */
  chirp::User StringToUser(std::string byte);
  /*
  Copy chirp to ChirpReply's chirp.
  */
  void SetChirpReply(chirp::Chirp *chirp, chirp::ChirpReply *response);

 private:
  KeyValueStoreInstance *kvstore; /* Key Value Store for Testing purpose */
  /*
    Mutex to lock chirps_ to ensure there is no duplicated chirp id.
  */
  std::mutex mymutex_;
  /*
    Helper function to get TimeStamp
  */
  void SetTimeStamp(std::time_t &seconds, int64_t &microseconds_since_epoch);
  /*
    Helper Function to check if chirp id for reply exist for chirp()
  */
  bool CheckIfReplyIDExist(const std::string &parent_id);
  /*
    Helper Function to get next chirp id.
  */
  std::string GetNextChirpID();

  /*
   Helper function for chirp() to handle potential hashtags in a chirp and add
   them to the KVS
 */
  void HandleChirpHashTag(const std::string &username, const std::string &text,
                          const std::string &parent_id,
                          const std::string &next_chirp_ID, std::time_t seconds,
                          int64_t microseconds_since_epoch);
  /* Finds and parses hashtags from the text of a chirp. returns true if there
   * is a chirp and fills the Hashtag proto */
  std::vector<std::string> ParseChirpForHashtag(
      const std::string &text, std::vector<chirp::Hashtag> *tags);
};
#endif /* SERVICE_TEST_SERVICELAYERINSTANCE_H*/
