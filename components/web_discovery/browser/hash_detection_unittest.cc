
/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/hash_detection.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace web_discovery {

TEST(WebDiscoveryHashDetectionTest, Basic) {
  EXPECT_FALSE(IsHashLikely("test"));
  EXPECT_FALSE(IsHashLikely("this is a test query"));

  EXPECT_FALSE(IsHashLikely("pneumonoultramicroscopicsilicovolcanoconiosis"));

  EXPECT_TRUE(IsHashLikely("N46iSNekpT:08ca45b7d7ea58ee:88dcbe4446168966a1"));
  EXPECT_TRUE(IsHashLikely("2btjjy78REtmYkkW0csHUbJZOstRXoWdX1mGrmmfeHI"));
}

}  // namespace web_discovery
