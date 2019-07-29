/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/media/soundcloud.h"

#include <map>
#include <utility>

#include "base/json/json_reader.h"

#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=MediaSoundCloudTest.*

namespace braveledger_media {

class MediaSoundCloudTest : public testing::Test {
 public:
  static std::string CreateTestResponse();
};

std::string MediaSoundCloudTest::CreateTestResponse() {
  return R"(
    <script>webpackJsonp"
      var c=[
        {
          "id": 64,
          "data": [{
            "avatar_url": "soundcloud.com/test.jpg",
            "id": 1234,
            "url": "https://soundcloud.com/jdkuki",
            "full_name": "Jakob Kuki",
            "username": "jdkuki"
          }]
        }
      ],o=Date.now()
    </script>
  )";
}

TEST(MediaSoundCloudTest, GetUserJSON) {
  std::string ref_json = R"({
    "avatar_url": "soundcloud.com/test.jpg",
    "id": 1234,
    "url": "https://soundcloud.com/jdkuki",
    "full_name": "Jakob Kuki",
    "username": "jdkuki"
  })";
  auto expected = base::JSONReader::Read(ref_json);
  std::string test_json = MediaSoundCloudTest::CreateTestResponse();
  std::string parsed = SoundCloud::GetUserJSON(test_json);
  auto actual = base::JSONReader::Read(parsed);
  ASSERT_TRUE(expected == actual);
}


TEST(MediaSoundCloudTest, GetUserName) {
  //empty
  std::string user_name = SoundCloud::GetUserName("");
  ASSERT_EQ(user_name, "");

  std::string test_json = MediaSoundCloudTest::CreateTestResponse();
  std::string parsed = SoundCloud::GetUserJSON(test_json);
  user_name = SoundCloud::GetUserName(parsed);

  ASSERT_EQ(user_name, "jdkuki");
}

TEST(MediaSoundCloudTest, GetBaseURL) {
  // empty
  std::string result =
      braveledger_media::SoundCloud::GetBaseURL("");
  ASSERT_TRUE(result.empty());

  result = braveledger_media::SoundCloud::GetBaseURL("/jdkuki");
  ASSERT_EQ(result, "jdkuki");

  result = braveledger_media::SoundCloud::GetBaseURL("/jdkuki/foo");
  ASSERT_EQ(result, "jdkuki");
}

TEST(MediaSoundCloudTest, GetUserId) {
  std::string test_response = MediaSoundCloudTest::CreateTestResponse();
  std::string test_json = SoundCloud::GetUserJSON(test_response);

  // empty
  std::string result =
      braveledger_media::SoundCloud::GetUserId("");
  ASSERT_TRUE(result.empty());

  // incorrect scrape
  result = braveledger_media::SoundCloud::
      GetUserId("some random text");
  ASSERT_TRUE(result.empty());

  // correct response
  result = braveledger_media::SoundCloud::
      GetUserId(test_json);
  ASSERT_EQ(result, "1234");
}

TEST(MediaSoundCloudTest, GetPublisherName) {
  std::string test_response = MediaSoundCloudTest::CreateTestResponse();
  std::string test_json = SoundCloud::GetUserJSON(test_response);

  // empty
  std::string result =
      braveledger_media::SoundCloud::GetPublisherName("");
  ASSERT_TRUE(result.empty());

  // incorrect scrape
  result = braveledger_media::SoundCloud::
      GetPublisherName("some random text");
  ASSERT_TRUE(result.empty());

  // correct response
  result = braveledger_media::SoundCloud::
      GetPublisherName(test_json);
  ASSERT_EQ(result, "Jakob Kuki");
}

TEST(MediaSoundCloudTest, GetProfileURL) {
  // empty
  std::string result =
      braveledger_media::SoundCloud::GetProfileURL("");
  ASSERT_TRUE(result.empty());

  result = braveledger_media::SoundCloud::GetProfileURL("jdkuki");
  ASSERT_EQ(result, "https://soundcloud.com/jdkuki");
}

TEST(MediaSoundCloudTest, GetPublisherKey) {
  // empty
  std::string result =
      braveledger_media::SoundCloud::GetPublisherKey("");
  ASSERT_TRUE(result.empty());

  result =
      braveledger_media::SoundCloud::GetPublisherKey("test_publisher_key");
  ASSERT_EQ(result, "soundcloud#channel:test_publisher_key");
}

TEST(MediaSoundCloudTest, GetProfileImageURL) {
  // empty
  std::string result =
      braveledger_media::SoundCloud::GetProfileImageURL("");
  ASSERT_TRUE(result.empty());

  std::string test_response = MediaSoundCloudTest::CreateTestResponse();
  std::string test_json = SoundCloud::GetUserJSON(test_response);

  result = braveledger_media::SoundCloud::GetProfileImageURL(test_json);
  ASSERT_EQ(result, "soundcloud.com/test.jpg");
}


TEST(MediaSoundCloudTest, GetJSONStringValue) {
  std::string test_response = MediaSoundCloudTest::CreateTestResponse();
  std::string test_json = SoundCloud::GetUserJSON(test_response);

  std::string result;

  // empty
  bool success =
      braveledger_media::SoundCloud::GetJSONStringValue("full_name", "", &result);
  ASSERT_FALSE(success);
  ASSERT_TRUE(result.empty());

  // correct response
  success =
      braveledger_media::SoundCloud::GetJSONStringValue("full_name", test_json,
          &result);
  ASSERT_TRUE(success);
  ASSERT_EQ(result, "Jakob Kuki");
}

TEST(MediaSoundCloudTest, GetJSONIntValue) {
  std::string test_response = MediaSoundCloudTest::CreateTestResponse();
  std::string test_json = SoundCloud::GetUserJSON(test_response);
  int64_t result;

  // empty
  bool success =
      braveledger_media::SoundCloud::GetJSONIntValue("id", "", &result);
  ASSERT_FALSE(success);

  // correct response
  success =
      braveledger_media::SoundCloud::GetJSONIntValue("id", test_json, &result);
  ASSERT_TRUE(success);
  ASSERT_EQ(result, 1234);
}

}  // namespace braveledger_media
