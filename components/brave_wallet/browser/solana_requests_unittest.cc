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

TEST(SolanaRequestsUnitTest, sendTransaction) {
  ASSERT_EQ(
      sendTransaction("signed_tx"),
      R"({"id":1,"jsonrpc":"2.0","method":"sendTransaction","params":["signed_tx",{"encoding":"base64"}]})");
}

TEST(SolanaRequestsUnitTest, getLatestBlockhash) {
  ASSERT_EQ(
      getLatestBlockhash(),
      R"({"id":1,"jsonrpc":"2.0","method":"getLatestBlockhash","params":[]})");
}

}  // namespace solana

}  // namespace brave_wallet
