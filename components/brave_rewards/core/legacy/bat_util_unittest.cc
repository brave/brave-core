/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/legacy/bat_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatUtilTest.*

namespace brave_rewards::internal {

class BatUtilTest : public testing::Test {};

TEST(BatUtilTest, ConvertToProbi) {
  // empty string
  std::string result = ConvertToProbi("");
  ASSERT_EQ(result, "0");

  // single dig int
  result = ConvertToProbi("5");
  ASSERT_EQ(result, "5000000000000000000");

  // two dig int
  result = ConvertToProbi("15");
  ASSERT_EQ(result, "15000000000000000000");

  // single dig decimal
  result = ConvertToProbi("5.4");
  ASSERT_EQ(result, "5400000000000000000");

  // two dig decimal
  result = ConvertToProbi("5.45");
  ASSERT_EQ(result, "5450000000000000000");
}

TEST(BatUtilTest, ProbiToDouble) {
  // empty string
  double result = ProbiToDouble("");
  ASSERT_EQ(result, 0);

  // wrong probi
  result = ProbiToDouble("10");
  ASSERT_EQ(result, 0);

  // full number probi
  result = ProbiToDouble("5000000000000000000");
  ASSERT_EQ(result, 5.0);

  // full number probi
  result = ProbiToDouble("1125600000000000000000");
  ASSERT_EQ(result, 1125.6);
}

}  // namespace brave_rewards::internal
