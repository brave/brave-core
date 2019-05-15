/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <memory>
#include <fstream>
#include <sstream>

#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"

#include "base/files/file_path.h"

using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;

namespace ads {

class AdsTabsTest : public ::testing::Test {
 protected:
  std::unique_ptr<MockAdsClient> mock_ads_client_;
  std::unique_ptr<AdsImpl> ads_;

  AdsTabsTest() :
      mock_ads_client_(std::make_unique<MockAdsClient>()),
      ads_(std::make_unique<AdsImpl>(mock_ads_client_.get())) {
  }

  ~AdsTabsTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    EXPECT_CALL(*mock_ads_client_, IsAdsEnabled())
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*mock_ads_client_, Load(_, _))
        .WillRepeatedly(
            Invoke([this](
                const std::string& name,
                OnLoadCallback callback) {
              auto path = GetTestDataPath();
              path = path.AppendASCII(name);

              std::string value;
              if (!Load(path, &value)) {
                callback(FAILED, value);
                return;
              }

              callback(SUCCESS, value);
            }));

    ON_CALL(*mock_ads_client_, Save(_, _, _))
        .WillByDefault(
            Invoke([](
                const std::string& name,
                const std::string& value,
                OnSaveCallback callback) {
              callback(SUCCESS);
            }));

    EXPECT_CALL(*mock_ads_client_, LoadUserModelForLocale(_, _))
        .WillRepeatedly(
            Invoke([this](
                const std::string& locale,
                OnLoadCallback callback) {
              auto path = GetResourcesPath();
              path = path.AppendASCII("locales");
              path = path.AppendASCII(locale);
              path = path.AppendASCII("user_model.json");

              std::string value;
              if (!Load(path, &value)) {
                callback(FAILED, value);
                return;
              }

              callback(SUCCESS, value);
            }));

    EXPECT_CALL(*mock_ads_client_, LoadJsonSchema(_))
        .WillRepeatedly(
            Invoke([this](
                const std::string& name) -> std::string {
              auto path = GetTestDataPath();
              path = path.AppendASCII(name);

              std::string value;
              Load(path, &value);

              return value;
            }));

    ads_->Initialize();
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case
  base::FilePath GetTestDataPath() {
    return base::FilePath(FILE_PATH_LITERAL(
        "brave/vendor/bat-native-ads/test/data"));
  }

  base::FilePath GetResourcesPath() {
    return base::FilePath(FILE_PATH_LITERAL(
        "brave/vendor/bat-native-ads/resources"));
  }

  bool Load(const base::FilePath path, std::string* value) {
    if (!value) {
      return false;
    }

    std::ifstream ifs{path.value().c_str()};
    if (ifs.fail()) {
      *value = "";
      return false;
    }

    std::stringstream stream;
    stream << ifs.rdbuf();
    *value = stream.str();
    return true;
  }
};

TEST_F(AdsTabsTest, Media_IsPlaying) {
  // Arrange
  ads_->OnMediaPlaying(1);

  // Act
  auto is_playing = ads_->IsMediaPlaying();

  // Assert
  EXPECT_TRUE(is_playing);
}

TEST_F(AdsTabsTest, Media_NotPlaying) {
  // Arrange
  ads_->OnMediaPlaying(1);
  ads_->OnMediaPlaying(2);

  ads_->OnMediaStopped(1);
  ads_->OnMediaStopped(2);

  // Act
  auto is_playing = ads_->IsMediaPlaying();

  // Assert
  EXPECT_FALSE(is_playing);
}

TEST_F(AdsTabsTest, TabUpdated_Incognito) {
  // Arrange
  auto expected_last_user_activity = ads_->client_->GetLastUserActivity();

  EXPECT_CALL(*mock_ads_client_, Save(_, _, _))
      .Times(0);

  EXPECT_CALL(*mock_ads_client_, EventLog(_))
      .Times(0);

  // Act
  ads_->TabUpdated(1, "https://brave.com", true, true);

  // Assert
  auto last_user_activity = ads_->client_->GetLastUserActivity();
  EXPECT_EQ(expected_last_user_activity, last_user_activity);
}

TEST_F(AdsTabsTest, TabUpdated_InactiveIncognito) {
  // Arrange
  auto expected_last_user_activity = ads_->client_->GetLastUserActivity();

  EXPECT_CALL(*mock_ads_client_, Save(_, _, _))
      .Times(0);

  EXPECT_CALL(*mock_ads_client_, EventLog(_))
      .Times(0);

  // Act
  ads_->TabUpdated(1, "https://brave.com", false, true);

  // Assert
  auto last_user_activity = ads_->client_->GetLastUserActivity();
  EXPECT_EQ(expected_last_user_activity, last_user_activity);
}

TEST_F(AdsTabsTest, TabUpdated_Active) {
  // Arrange
  auto last_user_activity = ads_->client_->GetLastUserActivity();

  EXPECT_CALL(*mock_ads_client_, Save(_, _, _))
      .Times(3);

  EXPECT_CALL(*mock_ads_client_, EventLog(_))
      .Times(1);

  // Act
  ads_->TabUpdated(1, "https://brave.com", true, false);

  // Assert
  auto updated_last_user_activity = ads_->client_->GetLastUserActivity();
  EXPECT_NE(last_user_activity, updated_last_user_activity);
}

TEST_F(AdsTabsTest, TabUpdated_Inactive) {
  // Arrange
  auto last_user_activity = ads_->client_->GetLastUserActivity();

  EXPECT_CALL(*mock_ads_client_, Save(_, _, _))
      .Times(1);

  EXPECT_CALL(*mock_ads_client_, EventLog(_))
      .Times(1);

  // Act
  ads_->TabUpdated(1, "https://brave.com", false, false);

  // Assert
  auto updated_last_user_activity = ads_->client_->GetLastUserActivity();
  EXPECT_NE(last_user_activity, updated_last_user_activity);
}

TEST_F(AdsTabsTest, TabClosed_WhileMediaIsPlaying) {
  // Arrange
  ads_->OnMediaPlaying(1);

  EXPECT_CALL(*mock_ads_client_, EventLog(_))
      .Times(1);

  // Act
  ads_->TabClosed(1);

  // Assert
  EXPECT_FALSE(ads_->IsMediaPlaying());
}

}  // namespace ads
