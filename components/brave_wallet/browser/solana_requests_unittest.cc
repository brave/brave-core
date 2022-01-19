/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_requests.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace solana {

TEST(SolanaRequestsUnitTest, getBalance) {
  ASSERT_EQ(
      getBalance("key"),
      R"({"id":1,"jsonrpc":"2.0","method":"getBalance","params":["key"]})");
}

TEST(SolanaRequestsUnitTest, getTokenAccountBalance) {
  ASSERT_EQ(
      getTokenAccountBalance("key"),
      R"({"id":1,"jsonrpc":"2.0","method":"getTokenAccountBalance","params":["key"]})");
}

}  // namespace solana

}  // namespace brave_wallet
