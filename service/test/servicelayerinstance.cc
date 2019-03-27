#include "servicelayerinstance.h"

ServiceLayerInstance::ServiceLayerInstance(KeyValueStoreInstance *kv_instance) {
  kvstore = kv_instance;
  if (kvstore->Get("chirps_").size() == 0) {
    kvstore->Put("chirps_", std::to_string(0));
  }
}

bool ServiceLayerInstance::RegisterUser(const std::string &username) {
  auto from_get = kvstore->Get(username);
  if (from_get.size() != 0) {
    return false;
  }
  std::string to_string;
  {
    chirp::User user_;
    user_.set_username(username);
    user_.SerializeToString(&to_string);
  }
  kvstore->Put(username, to_string);
  return true;
}

bool ServiceLayerInstance::Chirp(const std::string &username,
                                 const std::string &text,
                                 const std::string &parentid) {

  /* Get TimeStamp */
  std::time_t seconds;
  int64_t microseconds_since_epoch;
  SetTimeStamp(seconds, microseconds_since_epoch);
  /* Check if chirp ID exist */
  if (!CheckIfReplyIDExist(parentid)) {
    return false;
  }

  /* Get User and checks if it exist */
  chirp::User user;
  std::vector<std::string> fromget = kvstore->Get(username);
  if (fromget.size() == 0) {
    return false;
  }
  std::string user_string_val = fromget[0];
  user.ParseFromString(user_string_val);
  std::string user_key;

  /* Get next chirp id, and lock it! */
  std::string next_chirp_ID = GetNextChirpID();

  /* Add new Chirp to chirp::User */
  chirp::Chirp *message;
  std::string chirp_string_value;
  {
    message = user.add_chirps();
    message->set_username(username);
    message->set_text(text);
    message->set_id(next_chirp_ID);
    chirp::Timestamp *timestamp = message->mutable_timestamp();
    timestamp->set_seconds(static_cast<int64_t>(seconds));
    timestamp->set_useconds(microseconds_since_epoch);
    message->set_parent_id(parentid);
    message->SerializeToString(&chirp_string_value);
  }

  user.SerializeToString(&user_key);
  kvstore->Put(username, user_key); /*Add Chirp to chirp::User*/
  /*Add chirp::Chirp to database*/
  kvstore->Put("chirp" + next_chirp_ID, chirp_string_value);

  /* If its a reply to an existing chirp */
  if (parentid != "") {
    std::string reply_chirp_string_value;
    {
      std::vector<std::string> fromget = kvstore->Get("reply" + parentid);

      chirp::ChirpReplies replies;

      /* If chirp id is not found, use the 'replies' as a new ChirpReplies and
       * set parent_id() */
      if (fromget.size() == 0) {
        replies.set_parent_id(parentid);
      } else { /* If found, parse from existing ChirpReplies */
        replies.ParseFromString(fromget[0]);
      }
      chirp::Chirp *reply = replies.add_chirps();
      CopyChirp(reply, *message);
      replies.SerializeToString(&reply_chirp_string_value);
    }
    kvstore->Put("reply" + parentid,
                 reply_chirp_string_value); // Add reply<ID> to backend
  }
  return true;
}

bool ServiceLayerInstance::Follow(const std::string &username,
                                  const std::string &tofollow) {
  if (username == tofollow) {
    return false;
  }

  std::string user_value;
  {
    /* Check if user exist */
    chirp::User user;
    std::vector<std::string> fromget = kvstore->Get(username);
    if (fromget.size() == 0) {
      return false;
    }
    /* Check if tofollow exist in database */
    std::vector<std::string> follow_exist = kvstore->Get(tofollow);
    if (follow_exist.size() == 0) {
      return false;
    }

    /* Check if tofollow ALREADY exist */
    std::string getKey = fromget[0];
    user.ParseFromString(getKey);
    chirp::Followers *f = user.mutable_followers();
    for (int i = 0; i < f->username_size(); i++) {
      if (f->username(i) == tofollow) {
        return false;
      }
    }
    f->add_username(tofollow);
    user.SerializeToString(&user_value);
  }
  kvstore->Put(username, user_value);
  return true;
}

std::vector<chirp::Chirp>
ServiceLayerInstance::Read(const std::string &chirpid) {

  std::vector<std::string> fromget = kvstore->Get("chirp" + chirpid);
  std::vector<chirp::Chirp> chirp_to_send; // For sorting by timestamp
  if (fromget.size() == 0) {
    return chirp_to_send;
  }
  std::string main_chirp_msg = fromget[0];

  chirp::Chirp main_chirp;
  main_chirp.ParseFromString(main_chirp_msg);
  chirp_to_send.push_back(main_chirp);

  std::queue<chirp::Chirp> myqueue;
  myqueue.push(main_chirp);

  /* Running DFS to get threads related to chirp<id> */
  while (!myqueue.empty()) {
    chirp::Chirp currentChirp = myqueue.front();
    myqueue.pop();

    /* Find other chirps that are in reply to this currentChirp */
    fromget = kvstore->Get("reply" + currentChirp.id());
    if (fromget.size() == 0) {
      continue;
    }

    chirp::ChirpReplies chirp_replies = ConvertToChirpReplies(fromget[0]);
    for (int i = 0; i < chirp_replies.chirps_size(); i++) {
      myqueue.push(chirp_replies.chirps(i));
      chirp_to_send.push_back(chirp_replies.chirps(i));
    }
  }
  /* Sort by timestamp */
  std::sort(chirp_to_send.begin(), chirp_to_send.end(),
            [](chirp::Chirp &a, chirp::Chirp &b) {
              return a.timestamp().useconds() < b.timestamp().useconds();
            });

  return chirp_to_send;
}

std::vector<chirp::Chirp>
ServiceLayerInstance::Monitor(const std::string &username) {

  /* Get Current TimeStamp */
  std::time_t seconds = std::time(nullptr);
  int64_t seconds_since_epoch = static_cast<int64_t>(seconds);
  int64_t microseconds_since_epoch =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();
  std::vector<chirp::Chirp> chirps_to_send;
  auto fromget = kvstore->Get(username);
  if (fromget.size() == 0) {
    return chirps_to_send;
  }

  std::set<std::string> chirpsent;
  chirp::User user; /* Main User */
  user.ParseFromString(fromget[0]);

  chirp::MonitorReply reply;
  chirp::Followers followers = user.followers();

  /*
    Continuously look through all user's followers, and their chirps. Keep
    all sent chirps in a set called chirpsent. Keep looking for new chirps with
    this while loop.
  */
  int k = 0;
  while (k < 10) {
    for (int i = 0; i < followers.username_size(); i++) {
      auto allfollowers = kvstore->Get(followers.username(i));

      if (allfollowers.size() > 0) {
        chirp::User user_followers;
        std::string temp = allfollowers[0];
        user_followers.ParseFromString(temp);
        /* Loop through this User's follower's chirps */
        for (int j = 0; j < user_followers.chirps_size(); j++) {
          auto it = chirpsent.find(user_followers.chirps(j).id());
          /* Find this User's follower's chirp's id in the set. If it's not in
            the set, then its a new chirp */
          if (it == chirpsent.end()) {
            /* If this chirp is chirped after creating montior */
            if (user_followers.chirps(j).timestamp().useconds() >
                microseconds_since_epoch) {
              chirpsent.insert(user_followers.chirps(j).id());
              chirps_to_send.push_back(user_followers.chirps(j));
              reply.clear_chirp();
            }
          }
        }
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    k++;
  }
  return chirps_to_send;
}

void ServiceLayerInstance::CopyChirp(chirp::Chirp *chirp_,
                                     const chirp::Chirp &reply_) {
  chirp_->set_username(reply_.username());
  chirp_->set_text(reply_.text());
  chirp_->set_id(reply_.id());
  chirp_->set_parent_id(reply_.parent_id());
  chirp::Timestamp *time = chirp_->mutable_timestamp();
  time->set_seconds(reply_.timestamp().seconds());
  time->set_useconds(reply_.timestamp().useconds());
}

chirp::Chirp ServiceLayerInstance::ConvertToChirp(std::string byte) {
  chirp::Chirp chirp_;
  chirp_.ParseFromString(byte);
  return chirp_;
}

chirp::ChirpReplies
ServiceLayerInstance::ConvertToChirpReplies(std::string byte) {
  chirp::ChirpReplies replies;
  replies.ParseFromString(byte);
  return replies;
}

chirp::User ServiceLayerInstance::StringToUser(std::string byte) {
  chirp::User user;
  user.ParseFromString(byte);

  return user;
}

void ServiceLayerInstance::SetChirpReply(chirp::Chirp *chirp,
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

bool ServiceLayerInstance::CheckIfReplyIDExist(const std::string &parent_id) {
  if (parent_id != "") {
    auto from_get_function = kvstore->Get("chirp" + parent_id);
    if (from_get_function.size() == 0) {
      return false;
    }
  }
  return true;
}

void ServiceLayerInstance::SetTimeStamp(std::time_t &seconds,
                                        int64_t &microseconds_since_epoch) {
  seconds = std::time(nullptr);
  microseconds_since_epoch =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();
}

std::string ServiceLayerInstance::GetNextChirpID() {
  auto from_get_function = kvstore->Get("chirps_");
  std::string next_chirp_ID;
  if (from_get_function.size() == 0) {
    next_chirp_ID = std::to_string(0);
  } else {
    int curr_chirp_id = std::stoi(from_get_function[0]);
    curr_chirp_id++;
    next_chirp_ID = std::to_string(curr_chirp_id);
  }
  kvstore->Put("chirps_", next_chirp_ID);
  return next_chirp_ID;
}
