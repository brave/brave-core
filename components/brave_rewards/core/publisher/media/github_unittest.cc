/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/publisher/media/github.h"

#include <utility>

#include "brave/components/brave_rewards/core/constants.h"
#include "brave/components/brave_rewards/core/publisher/static_values.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=RewardsMediaGitHubTest.*

namespace brave_rewards::internal {

class RewardsMediaGitHubTest : public testing::Test {
 public:
  static std::string CreateTestJSONString();
};

std::string RewardsMediaGitHubTest::CreateTestJSONString() {
  return R"({
    "login": "jdkuki",
    "id": 8422122,
    "avatar_url": "https://avatars0.githubusercontent.com/u/8422122?v=4",
    "url": "https://api.github.com/users/jdkuki",
    "html_url": "https://github.com/jdkuki",
    "name": "Jakob Kuki"
  })";
}

TEST(RewardsMediaGitHubTest, GetLinkType) {
  // empty
  std::string result = GitHub::GetLinkType("");
  ASSERT_TRUE(result.empty());

  // wrong website
  result = GitHub::GetLinkType("https://twitter.com");
  ASSERT_TRUE(result.empty());

  // correct site
  result = GitHub::GetLinkType("https://github.com");
  ASSERT_EQ(result, GITHUB_MEDIA_TYPE);

  // sub domain
  result = GitHub::GetLinkType("https://gist.github.com");
  ASSERT_EQ(result, GITHUB_MEDIA_TYPE);

  // profile page
  result = GitHub::GetLinkType("https://github.com/jdkuki");
  ASSERT_EQ(result, GITHUB_MEDIA_TYPE);
}

TEST(RewardsMediaGitHubTest, GetProfileURL) {
  // empty
  std::string result = GitHub::GetProfileURL("");
  ASSERT_TRUE(result.empty());

  result = GitHub::GetProfileURL("jdkuki");
  ASSERT_EQ(result, "https://github.com/jdkuki");
}

TEST(RewardsMediaGitHubTest, GetProfileAPIURL) {
  // empty
  std::string result = GitHub::GetProfileURL("");
  ASSERT_TRUE(result.empty());

  result = GitHub::GetProfileAPIURL("jdkuki");
  ASSERT_EQ(result, "https://api.github.com/users/jdkuki");
}

TEST(RewardsMediaGitHubTest, GetProfileImageURL) {
  // empty
  std::string result = GitHub::GetProfileImageURL("");
  ASSERT_TRUE(result.empty());

  std::string test_response = RewardsMediaGitHubTest::CreateTestJSONString();

  result = GitHub::GetProfileImageURL(test_response);
  ASSERT_EQ(result, "https://avatars0.githubusercontent.com/u/8422122?v=4");
}

TEST(RewardsMediaGitHubTest, GetPublisherKey) {
  // empty
  std::string result = GitHub::GetPublisherKey("");
  ASSERT_TRUE(result.empty());

  result = GitHub::GetPublisherKey("test_publisher_key");
  ASSERT_EQ(result, "github#channel:test_publisher_key");
}

TEST(RewardsMediaGitHubTest, GetUserNameFromURL) {
  // empty
  std::string result = GitHub::GetUserNameFromURL("");
  ASSERT_TRUE(result.empty());

  // empty path
  result = GitHub::GetUserNameFromURL("/");
  ASSERT_TRUE(result.empty());

  // short path
  result = GitHub::GetUserNameFromURL("/jdkuki");
  ASSERT_EQ(result, "jdkuki");

  // long path
  result = GitHub::GetUserNameFromURL("/jdkuki/brave-core");
  ASSERT_EQ(result, "jdkuki");
}

TEST(RewardsMediaGitHubTest, GetUserName) {
  std::string test_response = RewardsMediaGitHubTest::CreateTestJSONString();

  // empty response
  std::string result = GitHub::GetUserName("");
  ASSERT_TRUE(result.empty());

  // valid response
  result = GitHub::GetUserName(test_response);
  ASSERT_EQ(result, "jdkuki");
}

TEST(RewardsMediaGitHubTest, GetUserId) {
  std::string test_response = RewardsMediaGitHubTest::CreateTestJSONString();

  // empty
  std::string result = GitHub::GetUserId("");
  ASSERT_TRUE(result.empty());

  // incorrect scrape
  result = GitHub::GetUserId("Some random text");
  ASSERT_TRUE(result.empty());

  // correct response
  result = GitHub::GetUserId(test_response);
  ASSERT_EQ(result, "8422122");
}

TEST(RewardsMediaGitHubTest, GetPublisherName) {
  std::string test_response = RewardsMediaGitHubTest::CreateTestJSONString();

  // empty
  std::string result = GitHub::GetPublisherName("");
  ASSERT_TRUE(result.empty());

  // incorrect scrape
  result = GitHub::GetPublisherName("some random text");
  ASSERT_TRUE(result.empty());

  // correct response
  result = GitHub::GetPublisherName(test_response);
  ASSERT_EQ(result, "Jakob Kuki");
}

TEST(RewardsMediaGitHubTest, GetJSONStringValue) {
  std::string test_response = RewardsMediaGitHubTest::CreateTestJSONString();
  std::string result;

  // empty
  bool success = GitHub::GetJSONStringValue("login", "", &result);
  ASSERT_FALSE(success);
  ASSERT_TRUE(result.empty());

  // correct response
  success = GitHub::GetJSONStringValue("login", test_response, &result);
  ASSERT_TRUE(success);
  ASSERT_EQ(result, "jdkuki");
}

TEST(RewardsMediaGitHubTest, GetJSONIntValue) {
  std::string test_response = RewardsMediaGitHubTest::CreateTestJSONString();
  int64_t result;

  // empty
  bool success = GitHub::GetJSONIntValue("id", "", &result);
  ASSERT_FALSE(success);

  // correct response
  success = GitHub::GetJSONIntValue("id", test_response, &result);
  ASSERT_TRUE(success);
  ASSERT_EQ(result, 8422122);
}

}  // namespace brave_rewards::internal
