/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/ledger.h"
#include "bat_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatHelperTest.*

TEST(BatHelperTest, isProbiValid) {
  // zero probi
  bool result = braveledger_bat_helper::isProbiValid("0");
  ASSERT_EQ(result, true);

  // 1 BAT
  result = braveledger_bat_helper::isProbiValid("1000000000000000000");
  ASSERT_EQ(result, true);

  // -1 BAT
  result = braveledger_bat_helper::isProbiValid("-1000000000000000000");
  ASSERT_EQ(result, true);

  // not correct probi
  result = braveledger_bat_helper::isProbiValid("10-00000000000000000");
  ASSERT_EQ(result, false);

  // not correct probi
  result = braveledger_bat_helper::isProbiValid("fds000000000");
  ASSERT_EQ(result, false);

  // not correct probi
  result = braveledger_bat_helper::isProbiValid(
      "100000000000000000010000000000000000001000000000000000000");
  ASSERT_EQ(result, false);
}
