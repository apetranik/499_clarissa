#include "servicelayerfunctionalities.h"

ServerForCommandLineClient::ServerForCommandLineClient() {
  ClientForKeyValueStore client_key(grpc::CreateChannel(
      "localhost:50000", grpc::InsecureChannelCredentials()));
  /* This is to preset the number of chirps for the chirp ID. "chirps_". */
  std::vector<std::string> from_get = client_key.get("chirps_");
  if (from_get.size() == 0) {
    client_key.put("chirps_", std::to_string(0));
  }
}
Status ServerForCommandLineClient::registeruser(ServerContext *context,
                                                const RegisterRequest *request,
                                                RegisterReply *response) {
  ClientForKeyValueStore client_key(grpc::CreateChannel(
      "localhost:50000", grpc::InsecureChannelCredentials()));
  /*
    Tested: Able to add new users
  */
  auto from_get_function = client_key.get(request->username());
  /* If this size is not 0, it means username already exist. */
  if (from_get_function.size() != 0) {
    return Status::CANCELLED;
  }
  std::string string_to_user_proto;
  {
    chirp::User user_proto;
    user_proto.set_username(request->username());
    user_proto.SerializeToString(&string_to_user_proto);
  }
  client_key.put(request->username(), string_to_user_proto);
  return Status::OK;
}
Status ServerForCommandLineClient::follow(ServerContext *context,
                                          const FollowRequest *request,
                                          FollowReply *response) {
  ClientForKeyValueStore client_key(grpc::CreateChannel(
      "localhost:50000", grpc::InsecureChannelCredentials()));
  /*
    Tested: Able to add followers to user with an empty followers list and if
    there exist a followers list
  */
  if (request->username() == request->to_follow()) {
    return Status::CANCELLED;
  }

  std::string string_to_user_proto;
  {
    chirp::User user;
    std::vector<std::string> from_get_function =
        client_key.get(request->username());
    /* Check if user exist */
    if (from_get_function.size() == 0) {
      return Status::CANCELLED;
    }

    /* Check if user tofollow exist in database*/
    std::vector<std::string> follow_exist =
        client_key.get(request->to_follow());
    if (follow_exist.size() == 0) {
      return Status::CANCELLED;
    }

    /* Check if tofollow ALREADY exist */
    std::string string_username = from_get_function[0];
    user.ParseFromString(string_username);
    chirp::Followers *f = user.mutable_followers();
    for (int i = 0; i < f->username_size(); i++) {
      if (f->username(i) == request->to_follow()) {
        return Status::CANCELLED;
      }
    }
    f->add_username(request->to_follow());
    user.SerializeToString(&string_to_user_proto);
  }

  client_key.put(request->username(), string_to_user_proto);
  return Status::OK;
}
Status ServerForCommandLineClient::chirp(ServerContext *context,
                                         const ChirpRequest *request,
                                         ChirpReply *response) {
  ClientForKeyValueStore client_key(grpc::CreateChannel(
      "localhost:50000", grpc::InsecureChannelCredentials()));
  /*
    Tested: Able to add chirps! "chip<id>" is key and message Proto Chirp is
    value
  */
  /* Get TimeStamp */
  std::time_t seconds;
  int64_t microseconds_since_epoch;
  SetTimeStamp(seconds, microseconds_since_epoch);

  /* Check if chirp ID exist */
  if (!CheckIfReplyIDExist(client_key, request)) {
    return Status::CANCELLED;
  }

  /*Get User*/
  chirp::User user;
  std::vector<std::string> from_get_function =
      client_key.get(request->username());
  if (from_get_function.size() == 0) {
    return Status::CANCELLED;
  }
  std::string getValue = from_get_function[0];
  user.ParseFromString(getValue);

  /* Get next chirp id & update*/
  std::string next_chirp_ID = GetNextChirpID(client_key);

  /*Add new Chirp to chirp::User*/
  /* TODO: Check for # in text and if so, put chirp in db under key -
   * "hashtag#___" */
  chirp::Chirp *message;
  std::string convert_string_chirp_to_proto;
  {
    message = user.add_chirps();
    message->set_username(request->username());
    message->set_text(request->text());
    message->set_id(next_chirp_ID);
    chirp::Timestamp *timestamp = message->mutable_timestamp();
    timestamp->set_seconds(static_cast<int64_t>(seconds));
    timestamp->set_useconds(microseconds_since_epoch);
    message->set_parent_id(request->parent_id());
    message->SerializeToString(&convert_string_chirp_to_proto);

    SetChirpReply(message, response);
  }

  /* Save new chirp & User with new chirp back to database */
  std::string string_user_to_proto;
  user.SerializeToString(&string_user_to_proto);

  /* Add Chirp to chirp::User */
  client_key.put(request->username(), string_user_to_proto);
  /* Add chirp::Chirp to database */
  client_key.put("chirp" + next_chirp_ID, convert_string_chirp_to_proto);

  /* If its a reply to an existing chirp, it will create a key called Reply<id>
   * and the value will be chirp::ChirpReplies which acts like an array of all
   * the chirps that are a reply to Reply<id> */
  if (request->parent_id() != "") {
    std::string reply_chirp_to_proto;
    {
      std::vector<std::string> from_get_function =
          client_key.get("reply" + request->parent_id());

      chirp::ChirpReplies replies;
      if (from_get_function.size() == 0) {
        replies.set_parent_id(request->parent_id());

        /* If key Reply<id> is found, it means there is already
         * chirp::ChirpReplies. Just add this new chirp to the array
         * ChirpReplies */
      } else {
        replies.ParseFromString(from_get_function[0]);
      }
      chirp::Chirp *reply = replies.add_chirps();
      CopyChirp(reply, *message);
      replies.SerializeToString(&reply_chirp_to_proto);
    }
    client_key.put("reply" + request->parent_id(), reply_chirp_to_proto);
  }

  /* Check and handle hashtags */
  HandleChirpHashTag(request->username(), request->text(), request->parent_id(),
                     next_chirp_ID, seconds, microseconds_since_epoch,
                     client_key);
  return Status::OK;
}
Status ServerForCommandLineClient::read(ServerContext *context,
                                        const ReadRequest *request,
                                        ReadReply *response) {
  /*
    Tested: Able to get all chirps and it's replies and send it back via
    ReadReply
  */
  ClientForKeyValueStore client_key(grpc::CreateChannel(
      "localhost:50000", grpc::InsecureChannelCredentials()));

  std::vector<std::string> from_get_function =
      client_key.get("chirp" + request->chirp_id());
  std::vector<chirp::Chirp> chirp_to_send; /* All the chirps that to be sent  */
  if (from_get_function.size() == 0) {
    return Status::CANCELLED;
  }
  std::string main_chirp_msg = from_get_function[0];

  chirp::Chirp *main_chirp = response->add_chirps(); /* Get the main chirp */
  main_chirp->ParseFromString(main_chirp_msg);
  chirp_to_send.push_back(*main_chirp);

  std::queue<chirp::Chirp> my_queue; /* queue for DFS */
  my_queue.push(*main_chirp);

  /*
    Running DFS to get threads related to chirp<id>
  */
  while (!my_queue.empty()) {
    chirp::Chirp current_chirp = my_queue.front();
    my_queue.pop();
    /*
      Find other chirps that are in reply to this current_chirp
    */
    from_get_function = client_key.get("reply" + current_chirp.id());
    if (from_get_function.size() == 0) {
      continue;
    }

    /* Save all the replies to the current chirp into vector, chirp_to_send */
    chirp::ChirpReplies chirp_replies =
        ConvertToChirpReplies(from_get_function[0]);
    for (int i = 0; i < chirp_replies.chirps_size(); i++) {
      my_queue.push(chirp_replies.chirps(i));
      chirp_to_send.push_back(chirp_replies.chirps(i));
    }
  }

  /* Sort by timestamp & then send the sorted list back to client */
  SortReadChirpByTimestamp(chirp_to_send, response, main_chirp);
  return Status::OK;
}
void ServerForCommandLineClient::SortReadChirpByTimestamp(
    std::vector<Chirp> &chirp_to_send, ReadReply *response,
    chirp::Chirp *main_chirp) {
  std::sort(chirp_to_send.begin(), chirp_to_send.end(),
            [](chirp::Chirp &a, chirp::Chirp &b) {
              return a.timestamp().useconds() < b.timestamp().useconds();
            });
  /* Send to Client */
  for (const auto &i : chirp_to_send) {
    if (i.id() == main_chirp->id()) {
      continue;
    }
    chirp::Chirp *c = response->add_chirps();
    CopyChirp(c, i);
  }
}
Status ServerForCommandLineClient::monitor(
    ServerContext *context, const MonitorRequest *request,
    ::grpc::ServerWriter<::chirp::MonitorReply> *writer) {
  /* Get Current TimeStamp */
  std::time_t seconds;
  int64_t microseconds_since_epoch;
  SetTimeStamp(seconds, microseconds_since_epoch);

  ClientForKeyValueStore client_key(grpc::CreateChannel(
      "localhost:50000", grpc::InsecureChannelCredentials()));

  auto from_get_function = client_key.get(request->username());
  if (from_get_function.size() == 0) {
    return Status::CANCELLED;
  }

  std::set<std::string> chirpsent; /* Save all the id of chirps was sent */
  chirp::User user;                /* Main User */
  user.ParseFromString(from_get_function[0]);

  chirp::MonitorReply reply;
  chirp::Followers followers = user.followers(); /* All followers of user */
  /*
    Continuously look through all user's followers, and their chirps. Keep all
    sent chirps in a set called chirpsent to disallow sending duplicates. Keep
    looking for new chirps with this while loop.
  */
  while (true) {
    if (context->IsCancelled()) {
      break;
    }
    for (int i = 0; i < followers.username_size(); i++) {
      auto all_followers = client_key.get(followers.username(i));

      if (all_followers.size() > 0) {
        chirp::User curr_follower;
        std::string temp = all_followers[0];
        curr_follower.ParseFromString(temp);

        /* Checking for new chirps: Loop through this User's follower's chirps
         */
        for (int j = 0; j < curr_follower.chirps_size(); j++) {
          auto it = chirpsent.find(curr_follower.chirps(j).id());

          /* Find this User's follower's chirp's id in the set. If it's not in
            the set, then its a new chirp */
          if (it == chirpsent.end()) {
            /* If this chirp is chirped after creating montior */
            if (curr_follower.chirps(j).timestamp().useconds() >
                microseconds_since_epoch) {
              chirpsent.insert(curr_follower.chirps(j).id());

              chirp::Chirp *c = reply.mutable_chirp();
              CopyChirp(c, curr_follower.chirps(j));
              writer->Write(reply);
              reply.clear_chirp();
            }
          }
        }
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
  return Status::OK;
}

Status ServerForCommandLineClient::stream(
    ServerContext *context, const StreamRequest *request,
    ::grpc::ServerWriter<::chirp::StreamReply> *writer) {
  /* Get Current TimeStamp */
  std::time_t seconds;
  int64_t microseconds_since_epoch;
  SetTimeStamp(seconds, microseconds_since_epoch);

  ClientForKeyValueStore client_key(grpc::CreateChannel(
      "localhost:50000", grpc::InsecureChannelCredentials()));

  auto from_get_function = client_key.get("hashtag#" + request->hashtag());
  if (from_get_function.size() == 0) {
    return Status::CANCELLED;
  }

  std::set<std::string> chirpsent; /* Save all the id of chirps was sent */
  chirp::Hashtag hashtag;
  hashtag.ParseFromString(from_get_function[0]);

  chirp::StreamReply reply;
  /*
    Continuously look through in db for #. Keep all
    sent chirps in a set called chirpsent to disallow sending duplicates. Keep
    looking for new chirps with this while loop.
  */
  while (true) {
    if (context->IsCancelled()) {
      break;
    }
    auto from_get_function = client_key.get("hashtag#" + request->hashtag());
    if (from_get_function.size() == 0) {
      break;
    }

    // Go thru all chirps under hashtag and check if they are new
    hashtag.ParseFromString(from_get_function[0]);
    for (int j = 0; j < hashtag.chirps_size(); j++) {
      if (hashtag.chirps(j).timestamp().useconds() > microseconds_since_epoch) {
        chirp::Chirp *message = reply.add_chirps();
        CopyChirp(message, hashtag.chirps(j));
      }
    }
    writer->Write(reply);
    reply.clear_chirps();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
  return Status::OK;
}
/*
  Below: Private Functions Implementations
*/
void ServerForCommandLineClient::CopyChirp(chirp::Chirp *chirp,
                                           const chirp::Chirp &chirp_to_copy) {
  chirp->set_username(chirp_to_copy.username());
  chirp->set_text(chirp_to_copy.text());
  chirp->set_id(chirp_to_copy.id());
  chirp->set_parent_id(chirp_to_copy.parent_id());
  chirp::Timestamp *time = chirp->mutable_timestamp();
  time->set_seconds(chirp_to_copy.timestamp().seconds());
  time->set_useconds(chirp_to_copy.timestamp().useconds());
}
chirp::Chirp ServerForCommandLineClient::ConvertToChirp(std::string byte) {
  chirp::Chirp chirp;
  chirp.ParseFromString(byte);
  return chirp;
}
chirp::ChirpReplies ServerForCommandLineClient::ConvertToChirpReplies(
    std::string byte) {
  chirp::ChirpReplies replies;
  replies.ParseFromString(byte);
  return replies;
}
chirp::User ServerForCommandLineClient::StringToUser(std::string byte) {
  chirp::User user;
  user.ParseFromString(byte);

  return user;
}
void ServerForCommandLineClient::SetChirpReply(chirp::Chirp *chirp,
                                               chirp::ChirpReply *response) {
  chirp::Chirp *new_chirp = response->mutable_chirp();
  new_chirp->set_username(chirp->username());
  new_chirp->set_text(chirp->text());
  new_chirp->set_id(chirp->id());
  new_chirp->set_parent_id(chirp->parent_id());
  chirp::Timestamp *timestamp = new_chirp->mutable_timestamp();
  timestamp->set_seconds(chirp->timestamp().seconds());
  timestamp->set_useconds(chirp->timestamp().useconds());
}
std::string ServerForCommandLineClient::GetNextChirpID(
    ClientForKeyValueStore &client_key) {
  auto from_get_function = client_key.get("chirps_");
  std::string next_chirp_ID;
  if (from_get_function.size() == 0) {
    next_chirp_ID = std::to_string(0);
  } else {
    int curr_chirp_id = std::stoi(from_get_function[0]);
    curr_chirp_id++;
    next_chirp_ID = std::to_string(curr_chirp_id);
  }
  client_key.put("chirps_", next_chirp_ID);
  return next_chirp_ID;
}
void ServerForCommandLineClient::SetTimeStamp(
    std::time_t &seconds, int64_t &microseconds_since_epoch) {
  seconds = std::time(nullptr);
  microseconds_since_epoch =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();
}
bool ServerForCommandLineClient::CheckIfReplyIDExist(
    ClientForKeyValueStore &client_key, const ChirpRequest *request) {
  if (request->parent_id() != "") {
    auto from_get_function = client_key.get("chirp" + request->parent_id());
    if (from_get_function.size() == 0) {
      return false;
    }
  }
  return true;
}

void ServerForCommandLineClient::HandleChirpHashTag(
    const std::string &username, const std::string &text,
    const std::string &parent_id, const std::string &next_chirp_ID,
    std::time_t seconds, int64_t microseconds_since_epoch,
    ClientForKeyValueStore &client_key) {
  /* Check for hashtag in chirp - if so add to KVS as a hashtag chirp */
  chirp::Hashtag tag;
  std::string hashtag = ParseChirpForHashtag(text, &tag, client_key);
  if (hashtag.empty()) return;

  // Add chirp to Hashtag
  std::string hashtag_chirp;
  chirp::Chirp *new_message;
  new_message = tag.add_chirps();
  new_message->set_username(username);
  new_message->set_text(text);
  new_message->set_id(next_chirp_ID);
  chirp::Timestamp *timestamp = new_message->mutable_timestamp();
  timestamp->set_seconds(static_cast<int64_t>(seconds));
  timestamp->set_useconds(microseconds_since_epoch);
  new_message->set_parent_id(parent_id);
  new_message->SerializeToString(&hashtag_chirp);

  std::string new_hashtag_chirp;
  tag.SerializeToString(&new_hashtag_chirp);
  client_key.put("hashtag" + hashtag, new_hashtag_chirp);
}

std::string ServerForCommandLineClient::ParseChirpForHashtag(
    const std::string &text, chirp::Hashtag *tag,
    ClientForKeyValueStore &client_key) {
  std::string hashtag = "";
  std::size_t start = text.find(" #");
  if (start != -1) {
    hashtag = text.substr(start + 1);    /* +1 gets rid of space */
    std::size_t end = hashtag.find(" "); /* cut off at end of hashtag */
    hashtag = hashtag.substr(0, end);

    std::vector<std::string> from_get_function =
        client_key.get("hashtag" + hashtag);
    if (from_get_function.size() != 0) {
      LOG(INFO) << "Hashtag has been used before" << std::endl;
      std::string getValue = from_get_function[0];
      tag->ParseFromString(getValue);
    }
  }
  return hashtag;
}
