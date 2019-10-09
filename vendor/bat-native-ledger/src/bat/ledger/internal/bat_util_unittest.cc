/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/bat_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatUtilTest.*

class BatUtilTest : public testing::Test {
};

TEST(BatUtilTest, ConvertToProbi) {
  // empty string
  std::string result = braveledger_bat_util::ConvertToProbi("");
  ASSERT_EQ(result, "0");

  // single dig int
  result = braveledger_bat_util::ConvertToProbi("5");
  ASSERT_EQ(result, "5000000000000000000");

  // two dig int
  result = braveledger_bat_util::ConvertToProbi("15");
  ASSERT_EQ(result, "15000000000000000000");

  // single dig decimal
  result = braveledger_bat_util::ConvertToProbi("5.4");
  ASSERT_EQ(result, "5400000000000000000");

  // two dig decimal
  result = braveledger_bat_util::ConvertToProbi("5.45");
  ASSERT_EQ(result, "5450000000000000000");
}
