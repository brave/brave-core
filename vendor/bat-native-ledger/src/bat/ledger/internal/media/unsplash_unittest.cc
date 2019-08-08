/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "bat/ledger/internal/media/unsplash.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=MediaUnsplashTest.*

namespace braveledger_media {

class MediaUnsplashTest : public testing::Test {
};

TEST(MediaUnsplashTest, GetProfileUrl) {
  std::string result =
      braveledger_media::Unsplash::GetProfileUrl(std::string());
  ASSERT_TRUE(result.empty());

  result = braveledger_media::Unsplash::GetProfileUrl("harleydavidson");
  ASSERT_EQ(result, "https://unsplash.com/@harleydavidson/");

  result = braveledger_media::Unsplash::GetProfileUrl("squareinc");
  ASSERT_EQ(result, "https://unsplash.com/@squareinc/");
}

TEST(MediaUnsplashTest, GetPublisherKey) {
  // empty
  std::string result =
      braveledger_media::Unsplash::GetPublisherKey(std::string());
  ASSERT_TRUE(result.empty());

  result =
      braveledger_media::Unsplash::GetPublisherKey("harleydavidson");
  ASSERT_EQ(result, "unsplash#channel:harleydavidson");

  result =
      braveledger_media::Unsplash::GetPublisherKey("squareinc");
  ASSERT_EQ(result, "unsplash#channel:squareinc");
}

TEST(MediaUnsplashTest, GetUserNameFromUrl) {
  // empty
  std::string result =
      braveledger_media::Unsplash::GetUserNameFromUrl(std::string());
  ASSERT_TRUE(result.empty());

  // home path
  result = braveledger_media::Unsplash::
      GetUserNameFromUrl("/");
  ASSERT_TRUE(result.empty());

  // simple path
  result = braveledger_media::Unsplash::
      GetUserNameFromUrl("/@squareinc");
  ASSERT_EQ(result, "squareinc");

  // /collections path
  result = braveledger_media::Unsplash::
      GetUserNameFromUrl("/@squareinc/collections");
  ASSERT_EQ(result, "squareinc");

  // /likes path
  result = braveledger_media::Unsplash::
      GetUserNameFromUrl("/@squareinc/likes");
  ASSERT_EQ(result, "squareinc");
}

TEST(MediaUnsplashTest, GetPublisherName) {
  // response empty
  std::string result =
      braveledger_media::Unsplash::GetPublisherName(std::string(),
                                                    "test_user_name");
  ASSERT_TRUE(result.empty());

  // user_name empty
  result = braveledger_media::Unsplash::GetPublisherName("test_response",
                                                    std::string());
  ASSERT_TRUE(result.empty());

  // invalid response/arbitrary text
  result = braveledger_media::Unsplash::
      GetPublisherName("arbitrary text", "test_user_name");
  ASSERT_TRUE(result.empty());

  // valid response
  const std::string response =
    "\"test_user_name\",\"name\":\"Harley Davidson Motorcycles\"";
  result = braveledger_media::Unsplash::
      GetPublisherName(response, "test_user_name");
  ASSERT_EQ(result, "Harley Davidson Motorcycles");
}

TEST(MediaUnsplashTest, GetProfileImageUrl) {
  // response empty
  std::string result =
    braveledger_media::Unsplash::GetProfileImageUrl(std::string());
  ASSERT_TRUE(result.empty());

  // invalid response/arbitrary text
  result =
    braveledger_media::Unsplash::GetProfileImageUrl("arbitrary text");
  ASSERT_TRUE(result.empty());

  // valid response
  const std::string response =
    "\"profile_image\":{\"small\":\"https://t.co/prof.jpg\",\"some_att\"";
  result =
    braveledger_media::Unsplash::GetProfileImageUrl(response);
  ASSERT_EQ(result, "https://t.co/prof.jpg");
}

}  // namespace braveledger_media
