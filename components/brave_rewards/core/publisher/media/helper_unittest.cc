/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/brave_rewards/core/publisher/media/helper.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=RewardsMediaHelperTest.*

namespace brave_rewards::internal {

class RewardsMediaHelperTest : public testing::Test {};

TEST(RewardsMediaHelperTest, GetMediaKey) {
  // provider is missing
  std::string result = GetMediaKey("key", "");
  ASSERT_EQ(result, "");

  // key is missing
  result = GetMediaKey("", "youtube");
  ASSERT_EQ(result, "");

  // all ok
  result = GetMediaKey("key", "youtube");
  ASSERT_EQ(result, "youtube_key");
}

TEST(RewardsMediaHelperTest, ExtractData) {
  //  // string empty
  std::string result = ExtractData("", "/", "!");
  ASSERT_EQ(result, "");

  // missing start
  result = ExtractData("st/find/me!", "", "!");
  ASSERT_EQ(result, "st/find/me");

  // missing end
  result = ExtractData("st/find/me!", "/", "");
  ASSERT_EQ(result, "find/me!");

  // all ok
  result = ExtractData("st/find/me!", "/", "!");
  ASSERT_EQ(result, "find/me");
}

}  // namespace brave_rewards::internal
