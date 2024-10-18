/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/common/publisher_utils.h"

#include <optional>
#include <string>

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_rewards {

namespace {

std::optional<std::string> GetPublisherId(const std::string& url) {
  return GetPublisherIdFromURL(GURL(url));
}

}  // namespace

class RewardsPublisherUtilsTest : public testing::Test {};

TEST(RewardsPublisherUtilsTest, IsMediaPlatformURL) {
  EXPECT_FALSE(IsMediaPlatformURL(GURL("https://brave.com")));
  EXPECT_FALSE(IsMediaPlatformURL(GURL("http://brave.com")));
  EXPECT_FALSE(IsMediaPlatformURL(GURL("https://search.brave.com")));

  EXPECT_FALSE(IsMediaPlatformURL(GURL("https://brave.co.uk")));
  EXPECT_FALSE(IsMediaPlatformURL(GURL("https://www.brave.co.uk")));

  EXPECT_FALSE(IsMediaPlatformURL(GURL("file:///a/b/c/")));
  EXPECT_FALSE(IsMediaPlatformURL(GURL("invalid-url")));
  EXPECT_FALSE(IsMediaPlatformURL(GURL("abc://twitter.com/foo")));

  EXPECT_TRUE(IsMediaPlatformURL(GURL("https://twitter.com/foo")));
  EXPECT_TRUE(IsMediaPlatformURL(GURL("https://www.twitter.com/foo")));
  EXPECT_TRUE(IsMediaPlatformURL(GURL("https://x.com/foo")));
  EXPECT_TRUE(IsMediaPlatformURL(GURL("https://www.x.com/foo")));
  EXPECT_TRUE(IsMediaPlatformURL(GURL("https://github.com/foo")));
  EXPECT_TRUE(IsMediaPlatformURL(GURL("https://reddit.com/foo")));
  EXPECT_TRUE(IsMediaPlatformURL(GURL("https://youtube.com/foo")));
  EXPECT_TRUE(IsMediaPlatformURL(GURL("https://vimeo.com/foo")));
  EXPECT_TRUE(IsMediaPlatformURL(GURL("https://twitch.tv/foo")));
}

TEST(RewardsPublisherUtilsTest, GetMediaPlatformFromPublisherId) {
  EXPECT_EQ(GetMediaPlatformFromPublisherId("youtube#channel:123").value(),
            "youtube");
  EXPECT_EQ(GetMediaPlatformFromPublisherId("reddit#channel:123").value(),
            "reddit");
  EXPECT_EQ(GetMediaPlatformFromPublisherId("github#channel:123").value(),
            "github");
  EXPECT_EQ(GetMediaPlatformFromPublisherId("twitch#author:123").value(),
            "twitch");
  EXPECT_EQ(GetMediaPlatformFromPublisherId("vimeo#channel:123").value(),
            "vimeo");
  EXPECT_EQ(GetMediaPlatformFromPublisherId("twitter#channel:123").value(),
            "twitter");
  EXPECT_EQ(GetMediaPlatformFromPublisherId("example.com"), std::nullopt);
}

TEST(RewardsPublisherUtilsTest, GetPublisherIdFromURL) {
  EXPECT_EQ(GetPublisherId("https://brave.com"), "brave.com");
  EXPECT_EQ(GetPublisherId("http://brave.com"), "brave.com");
  EXPECT_EQ(GetPublisherId("https://search.brave.com"), "brave.com");
  EXPECT_EQ(GetPublisherId("http://search.brave.com"), "brave.com");

  EXPECT_EQ(GetPublisherId("https://brave.co.uk"), "brave.co.uk");
  EXPECT_EQ(GetPublisherId("https://www.brave.co.uk"), "brave.co.uk");

  EXPECT_EQ(GetPublisherId("file:///a/b/c/"), std::nullopt);
  EXPECT_EQ(GetPublisherId("invalid-url"), std::nullopt);

  EXPECT_EQ(GetPublisherId("https://twitter.com/foo"), std::nullopt);
  EXPECT_EQ(GetPublisherId("https://github.com/foo"), std::nullopt);
  EXPECT_EQ(GetPublisherId("https://reddit.com/foo"), std::nullopt);
  EXPECT_EQ(GetPublisherId("https://youtube.com/foo"), std::nullopt);
  EXPECT_EQ(GetPublisherId("https://vimeo.com/foo"), std::nullopt);
  EXPECT_EQ(GetPublisherId("https://twitch.tv/foo"), std::nullopt);
}

}  // namespace brave_rewards
