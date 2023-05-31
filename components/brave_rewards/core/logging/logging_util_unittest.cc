/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/logging/logging_util.h"
#include "brave/components/brave_rewards/core/test/mock_ledger_test.h"

// npm run test -- brave_unit_tests --filter=LoggingUtilTest.*

namespace brave_rewards::internal {

class LoggingUtilTest : public MockLedgerTest {};

TEST_F(LoggingUtilTest, ShouldLogHeader) {
  EXPECT_TRUE(ShouldLogHeader("Content-Type: application/json; charset=UTF-8"));

  EXPECT_TRUE(ShouldLogHeader("Content-type: application/json; charset=UTF-8"));

  EXPECT_TRUE(ShouldLogHeader("digest: a527380a32beee78b46a"));

  EXPECT_TRUE(ShouldLogHeader("Digest: a527380a32beee78b46a"));

  EXPECT_FALSE(ShouldLogHeader("Authorization: Bearer a527380a32beee78b46a"));

  EXPECT_FALSE(ShouldLogHeader("authorization: Bearer a527380a32beee78b46a"));

  EXPECT_FALSE(ShouldLogHeader("Cookie: yummy_cookie=choco;"));

  EXPECT_FALSE(ShouldLogHeader("cookie: yummy_cookie=choco;"));
}

}  // namespace brave_rewards::internal
