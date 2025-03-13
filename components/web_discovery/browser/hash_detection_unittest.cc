/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/hash_detection.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace web_discovery {

TEST(WebDiscoveryHashDetectionTest, Basic) {
  EXPECT_FALSE(IsHashLikely(""));
  EXPECT_FALSE(IsHashLikely("test"));
  EXPECT_FALSE(IsHashLikely("this is a test query"));
  EXPECT_FALSE(IsHashLikely("best cake recipe"));
  EXPECT_FALSE(IsHashLikely("sushi restaurants"));
  EXPECT_FALSE(IsHashLikely("bagel shop in new york"));

  EXPECT_FALSE(IsHashLikely("pneumonoultramicroscopicsilicovolcanoconiosis"));

  EXPECT_TRUE(IsHashLikely("N46iSNekpT:08ca45b7d7ea58ee:88dcbe4446168966a1"));
  EXPECT_TRUE(IsHashLikely("2btjjy78REtmYkkW0csHUbJZOstRXoWdX1mGrmmfeHI"));

  EXPECT_FALSE(IsHashLikely("@"));
  EXPECT_FALSE(IsHashLikely("@!#$%^&*()_+"));

  EXPECT_FALSE(IsHashLikely("this quick fox!"));
  EXPECT_FALSE(IsHashLikely("this quicK fox!"));
  EXPECT_FALSE(IsHashLikely("this q!!uicK fox!"));
  EXPECT_FALSE(IsHashLikely("this q!!uicK fox!"));
  EXPECT_FALSE(
      IsHashLikely("\xe7\x8b\x90\xe5\x81\x87\xe8\x99\x8e\xe5\xa8\x81"));
  EXPECT_FALSE(
      IsHashLikely("this \xe7\x8b\x90\xe5\x81\x87\xe8\x99\x8e\xe5\xa8\x81"));
  EXPECT_FALSE(IsHashLikely(
      "this \xe7\x8b\x90\xe5\x81\x87\xe8\x99\x8e\xe5\xa8\x81 fox"));
}

}  // namespace web_discovery
