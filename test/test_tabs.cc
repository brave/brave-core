/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mock_ads_client.h"
#include "src/ads_impl.h"

using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::InSequence;

namespace ads {

void SuccessfullyLoadWithCallback(
    const std::string& name,
    OnLoadCallback callback) {
  std::string path = "mock_data/" + name;

  std::ifstream ifs{path};
  if (ifs.fail()) {
      callback(FAILED, "");
      return;
  }

  std::stringstream stream;
  stream << ifs.rdbuf();
  std::string value = stream.str();

  callback(SUCCESS, value);
}

void SuccessfullyLoadUserModelForLocale(
    const std::string& locale,
    OnLoadCallback callback) {
  std::string path = "resources/locales/" + locale + "/user_model.json";

  std::ifstream ifs{path};
  if (ifs.fail()) {
      callback(FAILED, "");
      return;
  }

  std::stringstream stream;
  stream << ifs.rdbuf();
  std::string value = stream.str();

  callback(SUCCESS, value);
}

const std::string SuccessfullyLoadJsonSchema(const std::string& name) {
  std::string path = "mock_data/" + name;

  std::ifstream ifs{path};
  if (ifs.fail()) {
    return "";
  }

  std::stringstream stream;
  stream << ifs.rdbuf();
  auto value = stream.str();

  return value;
}

class TabsTest : public ::testing::Test {
 protected:
  MockAdsClient* mock_ads_client;
  AdsImpl* ads;

  TabsTest() :
      mock_ads_client(new MockAdsClient()),
      ads(new AdsImpl(mock_ads_client)) {
    // You can do set-up work for each test here
  }

  ~TabsTest() override {
    // You can do clean-up work that doesn't throw exceptions here

    delete ads;
    delete mock_ads_client;
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    EXPECT_CALL(*mock_ads_client, IsAdsEnabled())
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_ads_client, Load(_, _))
        .WillRepeatedly(Invoke(SuccessfullyLoadWithCallback));
    EXPECT_CALL(*mock_ads_client, LoadUserModelForLocale(_, _))
        .WillRepeatedly(Invoke(SuccessfullyLoadUserModelForLocale));
    EXPECT_CALL(*mock_ads_client, LoadJsonSchema(_))
        .WillRepeatedly(Invoke(SuccessfullyLoadJsonSchema));
    EXPECT_CALL(*mock_ads_client, SetTimer(_))
        .WillOnce(Return(1));

    ads->Initialize();
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case
};

TEST_F(TabsTest, IsPlayingMedia) {
  ads->OnMediaPlaying(1);
  ads->OnMediaPlaying(2);
  EXPECT_TRUE(ads->IsMediaPlaying());

  ads->OnMediaStopped(1);
  EXPECT_TRUE(ads->IsMediaPlaying());
}

TEST_F(TabsTest, IsNotPlayingMedia) {
  ads->OnMediaPlaying(1);
  ads->OnMediaPlaying(2);

  ads->OnMediaStopped(1);
  ads->OnMediaStopped(2);

  EXPECT_FALSE(ads->IsMediaPlaying());
}

TEST_F(TabsTest, IncognitoTabUpdated) {
  EXPECT_CALL(*mock_ads_client, EventLog(_))
      .Times(0);

  uint64_t last_user_activity = ads->client_->GetLastUserActivity();
  ads->TabUpdated(1, "https://brave.com", true, true);
  uint64_t updated_last_user_activity = ads->client_->GetLastUserActivity();

  EXPECT_EQ(last_user_activity, updated_last_user_activity);
}

TEST_F(TabsTest, InactiveIncognitoTabUpdated) {
  EXPECT_CALL(*mock_ads_client, EventLog(_))
      .Times(0);

  uint64_t last_user_activity = ads->client_->GetLastUserActivity();
  ads->TabUpdated(1, "https://brave.com", false, true);
  uint64_t updated_last_user_activity = ads->client_->GetLastUserActivity();

  EXPECT_EQ(last_user_activity, updated_last_user_activity);
}

TEST_F(TabsTest, TabUpdated) {
  EXPECT_CALL(*mock_ads_client, EventLog(_))
      .Times(2);

  uint64_t last_user_activity = ads->client_->GetLastUserActivity();
  ads->TabUpdated(1, "https://brave.com", true, false);
  uint64_t updated_last_user_activity = ads->client_->GetLastUserActivity();

  EXPECT_NE(last_user_activity, updated_last_user_activity);
}

TEST_F(TabsTest, InactiveTabUpdated) {
  EXPECT_CALL(*mock_ads_client, EventLog(_))
      .Times(2);

  uint64_t last_user_activity = ads->client_->GetLastUserActivity();
  ads->TabUpdated(1, "https://brave.com", false, false);
  uint64_t updated_last_user_activity = ads->client_->GetLastUserActivity();

  EXPECT_NE(last_user_activity, updated_last_user_activity);
}

TEST_F(TabsTest, TabClosed) {
  EXPECT_CALL(*mock_ads_client, EventLog(_))
      .Times(1);

  ads->TabClosed(1);
}

TEST_F(TabsTest, TabClosedWhileMediaIsPlaying) {
  EXPECT_CALL(*mock_ads_client, EventLog(_))
      .Times(1);

  ads->OnMediaPlaying(1);
  ads->TabClosed(1);

  EXPECT_FALSE(ads->IsMediaPlaying());
}

}  // namespace ads
