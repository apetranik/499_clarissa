#include <thread>

#include <gtest/gtest.h>
#include "chirp.grpc.pb.h"

#include "servicelayerinstance.h"
#include "keyvaluestoreinstance.h"
/*
  Add a single user
*/
TEST(RegisterUser, SimpleTest) {
  KeyValueStoreInstance kvstore;
  ServiceLayerInstance s_layer(&kvstore);
  s_layer.RegisterUser("user1");

  auto uservalue = kvstore.Get("user1");
  chirp::User user_ = s_layer.StringToUser(uservalue[0]);
  EXPECT_EQ(user_.username(),"user1");
}
/*
  Add existing username
*/
TEST(RegisterUser, DuplicatedUserTest) {
  KeyValueStoreInstance kvstore;
  ServiceLayerInstance s_layer(&kvstore);
  s_layer.RegisterUser("user1");

  auto uservalue = kvstore.Get("user1");
  chirp::User user_ = s_layer.StringToUser(uservalue[0]);
  EXPECT_EQ(user_.username(),"user1");

  bool success = s_layer.RegisterUser("user1");
  EXPECT_EQ(false, success);
}
/*
  Add multiple users
*/
TEST(RegisterUser, MulipleUserTest) {
  KeyValueStoreInstance kvstore;
  ServiceLayerInstance s_layer(&kvstore);
  s_layer.RegisterUser("user1");
  s_layer.RegisterUser("user2");

  auto uservalue = kvstore.Get("user1");
  chirp::User user_ = s_layer.StringToUser(uservalue[0]);
  EXPECT_EQ(user_.username(),"user1");

  uservalue = kvstore.Get("user2");
  user_ = s_layer.StringToUser(uservalue[0]);
  EXPECT_EQ(user_.username(),"user2");
}
/*
  Follow a single user
*/
TEST(ToFollow, SimpleTest) {
  KeyValueStoreInstance kvstore;
  ServiceLayerInstance s_layer(&kvstore);
  s_layer.RegisterUser("user1");
  s_layer.RegisterUser("user2");

  auto uservalue = kvstore.Get("user1");
  chirp::User user_ = s_layer.StringToUser(uservalue[0]);
  EXPECT_EQ(user_.username(),"user1");

  s_layer.Follow("user2", "user1");

  uservalue = kvstore.Get("user2");
  user_ = s_layer.StringToUser(uservalue[0]);
  EXPECT_EQ(user_.username(),"user2");
  EXPECT_EQ(user_.followers().username_size(), 1);
  EXPECT_EQ(user_.followers().username(0), "user1");
}
/*
  Following the same user twice should not add duplicates
*/
TEST(ToFollow, FollowSameUserTwice) {
  KeyValueStoreInstance kvstore;
  ServiceLayerInstance s_layer(&kvstore);
  s_layer.RegisterUser("user1");
  s_layer.RegisterUser("user2");

  auto uservalue = kvstore.Get("user1");
  chirp::User user_ = s_layer.StringToUser(uservalue[0]);
  EXPECT_EQ(user_.username(),"user1");

  s_layer.Follow("user2", "user1");
  s_layer.Follow("user2", "user1");

  uservalue = kvstore.Get("user2");
  user_ = s_layer.StringToUser(uservalue[0]);
  EXPECT_EQ(user_.username(),"user2");
  EXPECT_EQ(user_.followers().username_size(), 1);
  EXPECT_EQ(user_.followers().username(0), "user1");
}
/*
  Following multiple different users
*/
TEST(ToFollow, MultipleUsers) {
  KeyValueStoreInstance kvstore;
  ServiceLayerInstance s_layer(&kvstore);
  s_layer.RegisterUser("user1");
  s_layer.RegisterUser("user2");
  s_layer.RegisterUser("user3");
  auto uservalue = kvstore.Get("user1");
  chirp::User user_ = s_layer.StringToUser(uservalue[0]);
  EXPECT_EQ(user_.username(),"user1");

  s_layer.Follow("user2", "user1");
  s_layer.Follow("user2", "user3");

  uservalue = kvstore.Get("user2");
  user_ = s_layer.StringToUser(uservalue[0]);
  EXPECT_EQ(user_.username(),"user2");
  EXPECT_EQ(user_.followers().username_size(), 2);
  EXPECT_EQ(user_.followers().username(0), "user1");
  EXPECT_EQ(user_.followers().username(1), "user3");
}
/*
  User having a simple chirp
*/
TEST(ChirpTest, SimpleTest) {
  KeyValueStoreInstance kvstore;
  ServiceLayerInstance s_layer(&kvstore);
  s_layer.RegisterUser("user1");

  auto uservalue = kvstore.Get("user1");
  chirp::User user_ = s_layer.StringToUser(uservalue[0]);
  EXPECT_EQ(user_.username(),"user1");

  chirp::ChirpReply* chirp_reply;
  s_layer.Chirp("user1", "testing this chirp", "");

  auto from_get = kvstore.Get("chirp1");
  chirp::Chirp chirp_ = s_layer.ConvertToChirp(from_get[0]);
  EXPECT_EQ(chirp_.username(), "user1");
  EXPECT_EQ(chirp_.text(), "testing this chirp");
}
/*
  User chirpping more than once
*/
TEST(ChirpTest, MultipleChirps) {
  KeyValueStoreInstance kvstore;
  ServiceLayerInstance s_layer(&kvstore);
  s_layer.RegisterUser("user1");

  auto uservalue = kvstore.Get("user1");
  chirp::User user_ = s_layer.StringToUser(uservalue[0]);
  EXPECT_EQ(user_.username(),"user1");

  chirp::ChirpReply* chirp_reply;
  s_layer.Chirp("user1", "testing this chirp", "");
  s_layer.Chirp("user1", "second chirp test", "1");

  auto from_get = kvstore.Get("chirp1");
  chirp::Chirp chirp_ = s_layer.ConvertToChirp(from_get[0]);
  EXPECT_EQ(chirp_.username(), "user1");
  EXPECT_EQ(chirp_.text(), "testing this chirp");

  from_get = kvstore.Get("chirp2");
  chirp_ = s_layer.ConvertToChirp(from_get[0]);
  EXPECT_EQ(chirp_.username(), "user1");
  EXPECT_EQ(chirp_.text(), "second chirp test");
}
/*
  User can read a chirp thread (3 chirps in a thread)
*/
TEST(ReadTest, SimpleTest) {
  KeyValueStoreInstance kvstore;
  ServiceLayerInstance s_layer(&kvstore);
  s_layer.RegisterUser("user1");

  auto uservalue = kvstore.Get("user1");
  chirp::User user_ = s_layer.StringToUser(uservalue[0]);
  EXPECT_EQ(user_.username(),"user1");

  chirp::ChirpReply* chirp_reply;
  s_layer.Chirp("user1", "testing this chirp", "");
  s_layer.Chirp("user1", "second chirp test", "1");
  s_layer.Chirp("user1", "replying to thread", "1");

  auto vector_of_chirps = s_layer.Read("1");
  EXPECT_EQ(3, vector_of_chirps.size());
  EXPECT_EQ("testing this chirp", vector_of_chirps[0].text());
  EXPECT_EQ("second chirp test", vector_of_chirps[1].text());
  EXPECT_EQ("replying to thread", vector_of_chirps[2].text());
}
/*
  User trying to read something that doesn't exist
*/
TEST(ReadTest, NullTest) {
  KeyValueStoreInstance kvstore;
  ServiceLayerInstance s_layer(&kvstore);
  s_layer.RegisterUser("user1");

  auto uservalue = kvstore.Get("user1");
  chirp::User user_ = s_layer.StringToUser(uservalue[0]);
  EXPECT_EQ(user_.username(),"user1");

  auto vector_of_chirps = s_layer.Read("1");
  EXPECT_EQ(0, vector_of_chirps.size());
}
/*
  User able to read multiple different threads
*/
TEST(ReadTest, MultipleThreads) {
  KeyValueStoreInstance kvstore;
  ServiceLayerInstance s_layer(&kvstore);
  s_layer.RegisterUser("user1");

  auto uservalue = kvstore.Get("user1");
  chirp::User user_ = s_layer.StringToUser(uservalue[0]);
  EXPECT_EQ(user_.username(),"user1");

  chirp::ChirpReply* chirp_reply;
  s_layer.Chirp("user1", "testing this chirp", "");
  s_layer.Chirp("user1", "second chirp test", "1");
  s_layer.Chirp("user1", "replying to thread", "1");
  s_layer.Chirp("user1", "Separate thread", "");
  s_layer.Chirp("user1", "reply to second thread ", "4");

  auto vector_of_chirps = s_layer.Read("1");
  EXPECT_EQ(3, vector_of_chirps.size());
  EXPECT_EQ("testing this chirp", vector_of_chirps[0].text());
  EXPECT_EQ("second chirp test", vector_of_chirps[1].text());
  EXPECT_EQ("replying to thread", vector_of_chirps[2].text());

  vector_of_chirps = s_layer.Read("4");
  EXPECT_EQ(2, vector_of_chirps.size());
  EXPECT_EQ("Separate thread", vector_of_chirps[0].text());
  EXPECT_EQ("reply to second thread ", vector_of_chirps[1].text());
}
/*
  User able to read a more complex thread.
*/
TEST(ReadTest, ComplexThreads) {
  KeyValueStoreInstance kvstore;
  ServiceLayerInstance s_layer(&kvstore);
  s_layer.RegisterUser("user1");

  auto uservalue = kvstore.Get("user1");
  chirp::User user_ = s_layer.StringToUser(uservalue[0]);
  EXPECT_EQ(user_.username(),"user1");

  chirp::ChirpReply* chirp_reply;
  s_layer.Chirp("user1", "testing this chirp", "");
  s_layer.Chirp("user1", "second chirp test", "1");
  s_layer.Chirp("user1", "replying to thread", "1");
  s_layer.Chirp("user1", "Separate thread", "3");
  s_layer.Chirp("user1", "reply to second thread ", "4");
  s_layer.Chirp("user1", "complex thread ", "2");

  auto vector_of_chirps = s_layer.Read("1");
  EXPECT_EQ(6, vector_of_chirps.size());
  EXPECT_EQ("testing this chirp", vector_of_chirps[0].text());
  EXPECT_EQ("second chirp test", vector_of_chirps[1].text());
  EXPECT_EQ("replying to thread", vector_of_chirps[2].text());
  EXPECT_EQ("Separate thread", vector_of_chirps[3].text());
  EXPECT_EQ("reply to second thread ", vector_of_chirps[4].text());
  EXPECT_EQ("complex thread ", vector_of_chirps[5].text());
}
/*

*/
void RunMonitorAutomaticChirpGenerator(KeyValueStoreInstance *kv) {
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ServiceLayerInstance s_layer(kv);
  for (int i = 0; i < 10; i++) {
    s_layer.Chirp("user2", "chirping "+std::to_string(i), "");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}
/*
  Monitor test: create another thread that acts as a user chirping
*/
TEST(MonitorTest, SimpleTest) {
  KeyValueStoreInstance* kvstore = new KeyValueStoreInstance;
  ServiceLayerInstance s_layer(kvstore);
  s_layer.RegisterUser("user1");
  s_layer.RegisterUser("user2");
  s_layer.Follow("user1", "user2");
  std::thread second(RunMonitorAutomaticChirpGenerator, kvstore);
  auto recieved_monitor_chirp = s_layer.Monitor("user1");
  second.join();
  EXPECT_NE(0, recieved_monitor_chirp.size());
  // std::this_thread::sleep_for(std::chrono::milliseconds(2000));

}


int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
