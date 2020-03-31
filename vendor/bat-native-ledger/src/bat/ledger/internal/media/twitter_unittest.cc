/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/media/twitter.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=MediaTwitterTest.*

namespace braveledger_media {

class MediaTwitterTest : public testing::Test {
};

TEST(MediaTwitterTest, GetProfileURL) {
  // screen name and user id are both empty
  std::string result = braveledger_media::Twitter::GetProfileURL("", "");
  ASSERT_EQ(result, "");

  // screen name - all good
  result = braveledger_media::Twitter::GetProfileURL("emerick", "");
  ASSERT_EQ(result, "https://twitter.com/emerick/");

  // user id - all good
  result = braveledger_media::Twitter::GetProfileURL("", "123");
  ASSERT_EQ(result, "https://twitter.com/intent/user?user_id=123");

  // will default to user id - all good
  result = braveledger_media::Twitter::GetProfileURL("emerick", "123");
  ASSERT_EQ(result, "https://twitter.com/intent/user?user_id=123");
}

TEST(MediaTwitterTest, GetProfileImageURL) {
  // screen_name is empty
  std::string result = braveledger_media::Twitter::GetProfileImageURL("");
  ASSERT_EQ(result, "");

  // all good
  result = braveledger_media::Twitter::GetProfileImageURL("emerick");
  ASSERT_EQ(result, "https://twitter.com/emerick/profile_image?size=original");
}

TEST(MediaTwitterTest, GetShareURLWithoutQuotedTweet) {
  std::map<std::string, std::string> args;
  args["comment"] =
      "I just tipped @emerick using the Brave browser. Check it out at "
      "https://brave.com/tips.";
  args["name"] = "emerick";
  args["hashtag"] = "TipWithBrave";
  std::string result = braveledger_media::Twitter::GetShareURL(args);
  ASSERT_EQ(result,
            "https://twitter.com/intent/tweet?text=I just tipped @emerick "
            "using the Brave browser. Check it out at "
            "https://brave.com/tips.%20%23TipWithBrave");
}

TEST(MediaTwitterTest, GetShareURLWithQuotedTweet) {
  std::map<std::string, std::string> args;
  args["comment"] =
      "I just tipped @emerick using the Brave browser. Check it out at "
      "https://brave.com/tips.";
  args["name"] = "emerick";
  args["hashtag"] = "TipWithBrave";
  args["tweet_id"] = "215559040011481088";
  std::string result = braveledger_media::Twitter::GetShareURL(args);
  ASSERT_EQ(result,
            "https://twitter.com/intent/tweet?text=I just tipped @emerick "
            "using the Brave browser. Check it out at "
            "https://brave.com/tips.%20%23TipWithBrave&url=https://twitter.com/"
            "emerick/status/215559040011481088");
}

TEST(MediaTwitterTest, GetPublisherKey) {
  // key is empty
  std::string result = braveledger_media::Twitter::GetPublisherKey("");
  ASSERT_EQ(result, "");

  // all good
  result = braveledger_media::Twitter::GetPublisherKey("213234");
  ASSERT_EQ(result, "twitter#channel:213234");
}

TEST(MediaTwitterTest, GetMediaKey) {
  // screen_name is empty
  std::string result = braveledger_media::Twitter::GetMediaKey("");
  ASSERT_EQ(result, "");

  // all good
  result = braveledger_media::Twitter::GetMediaKey("emerick");
  ASSERT_EQ(result, "twitter_emerick");
}

TEST(MediaTwitterTest, GetUserNameFromUrl) {
  // screen_name is empty
  std::string result = braveledger_media::Twitter::GetUserNameFromUrl("");
  ASSERT_EQ(result, "");

  // empty path
  result = braveledger_media::Twitter::
      GetUserNameFromUrl("/");
  ASSERT_EQ(result, "");

  // simple path
  result = braveledger_media::Twitter::
      GetUserNameFromUrl("/emerick");
  ASSERT_EQ(result, "emerick");

  // long path
  result = braveledger_media::Twitter::
      GetUserNameFromUrl("/emerick/news");
  ASSERT_EQ(result, "emerick");

  // web intent path
  result = braveledger_media::Twitter::GetUserNameFromUrl(
      "intent/user?screen_name=emerick");
  ASSERT_EQ(result, "emerick");
}

TEST(MediaTwitterTest, IsExcludedPath) {
  // path is empty
  bool result = braveledger_media::Twitter::IsExcludedPath("");
  ASSERT_EQ(result, true);

  // path is simple excluded link
  result = braveledger_media::Twitter::IsExcludedPath("/home");
  ASSERT_EQ(result, true);

  // path is simple excluded link with trailing /
  result = braveledger_media::Twitter::IsExcludedPath("/home/");
  ASSERT_EQ(result, true);

  // path is complex excluded link
  result =
      braveledger_media::Twitter::IsExcludedPath("/i/");
  ASSERT_EQ(result, true);

  // path is complex excluded link two levels
  result = braveledger_media::Twitter::IsExcludedPath("/i/settings");
  ASSERT_EQ(result, true);

  // path contains excluded screen name in query string
  result =
      braveledger_media::Twitter::IsExcludedPath("/foo?screen_name=home");
  ASSERT_EQ(result, true);

  // path has trailing / and contains excluded screen name in query string
  result = braveledger_media::Twitter::IsExcludedPath("/foo/?screen_name=home");
  ASSERT_EQ(result, true);

  // path is random link
  result = braveledger_media::Twitter::IsExcludedPath("/asdfs/asdfasdf/");
  ASSERT_EQ(result, false);

  // path is not excluded link
  result = braveledger_media::Twitter::IsExcludedPath("/emerick");
  ASSERT_EQ(result, false);

  // path has no screen name in query string
  result = braveledger_media::Twitter::IsExcludedPath("/asdfs?foo=asdfasdf");
  ASSERT_EQ(result, false);

  // path contains a non-excluded screen name in query string
  result =
      braveledger_media::Twitter::IsExcludedPath("/foo?screen_name=emerick");
  ASSERT_EQ(result, false);
}

TEST(MediaTwitterTest, GetUserId) {
  const char profile_old[] =
      "<div class=\"wrapper\">"
      "<div class=\"ProfileNav\" role=\"navigation\" data-user-id=\"123\">"
      "emerick</div></div>";
  const char profile_new[] =
      "<div class=\"wrapper\">"
      "<img src=\"https://pbs.twimg.com/profile_banners/123/profile.jpg\" />"
      "</div>";

  // response is empty
  std::string result = braveledger_media::Twitter::GetUserId("");
  ASSERT_EQ(result, "");

  // html is not correct
  result =
      braveledger_media::Twitter::GetUserId("<div>Hi</div>");
  ASSERT_EQ(result, "");

  // support for current Twitter
  result =
      braveledger_media::Twitter::GetUserId(profile_old);
  ASSERT_EQ(result, "123");

  // support for new Twitter
  result =
      braveledger_media::Twitter::GetUserId(profile_new);
  ASSERT_EQ(result, "123");
}

TEST(MediaTwitterTest, GetPublisherName) {
  // response is empty
  std::string result =
      braveledger_media::Twitter::GetPublisherName("");
  ASSERT_EQ(result, "");

  // without twitter
  result = braveledger_media::Twitter::
      GetPublisherName("<title>Hi</title>");
  ASSERT_EQ(result, "Hi");

  // current twitter
  result = braveledger_media::Twitter::
      GetPublisherName("<title>Name (@emerick) / Twitter</title>");
  ASSERT_EQ(result, "Name");

  // new twitter
  result = braveledger_media::Twitter::
      GetPublisherName("<title>My Name (@emerick) | Twitter</title>");
  ASSERT_EQ(result, "My Name");
}

}  // namespace braveledger_media
