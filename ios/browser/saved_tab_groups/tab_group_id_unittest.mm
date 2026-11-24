// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/saved_tab_groups/tab_group_id.h"

#include <algorithm>

#include "base/containers/span.h"
#import "brave/ios/browser/saved_tab_groups/brave_tab_group_id.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace brave {
namespace {

constexpr uint8_t kUuidBytes[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab,
                                    0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98,
                                    0x76, 0x54, 0x32, 0x10};

}  // namespace

using TabGroupIdTest = PlatformTest;

TEST_F(TabGroupIdTest, UuidBytesRoundTrip) {
  const tab_groups::TabGroupId group_id =
      TabGroupIdFromUuidBytes(base::span<const uint8_t, 16>(kUuidBytes));
  EXPECT_EQ(group_id.token().high(), 0x0123456789abcdefULL);
  EXPECT_EQ(group_id.token().low(), 0xfedcba9876543210ULL);

  uint8_t bytes[16] = {};
  UuidBytesFromTabGroupId(group_id, base::span<uint8_t, 16>(bytes));
  EXPECT_TRUE(
      std::equal(std::begin(bytes), std::end(bytes), std::begin(kUuidBytes)));
}

TEST_F(TabGroupIdTest, ObjCRoundTrip) {
  NSUUID* uuid = [[NSUUID alloc] initWithUUIDBytes:kUuidBytes];
  uint64_t high = 0;
  uint64_t low = 0;
  ASSERT_TRUE([BraveTabGroupID tokenForGroupID:uuid high:&high low:&low]);
  EXPECT_EQ(high, 0x0123456789abcdefULL);
  EXPECT_EQ(low, 0xfedcba9876543210ULL);

  NSUUID* restored = [BraveTabGroupID groupIDForTokenHigh:high low:low];
  uint8_t bytes[16] = {};
  [restored getUUIDBytes:bytes];
  EXPECT_TRUE(
      std::equal(std::begin(bytes), std::end(bytes), std::begin(kUuidBytes)));
}

TEST_F(TabGroupIdTest, SyncTabIDIsPositiveAndUnique) {
  const int32_t first = MakeSyncTabID();
  const int32_t second = MakeSyncTabID();
  EXPECT_GT(first, 0);
  EXPECT_GT(second, 0);
  EXPECT_NE(first, second);
}

}  // namespace brave
