/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/publisher_utils.h"

#include <optional>
#include <string>

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_rewards {

class RewardsPublisherUtilsTest : public testing::Test {
 protected:
  std::optional<std::string> GetPublisherId(const std::string& url) {
    return GetPublisherIdFromURL(GURL(url));
  }
};

TEST_F(RewardsPublisherUtilsTest, GetPublisherIdFromURL) {
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
