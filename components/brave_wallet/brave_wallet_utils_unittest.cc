/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/brave_wallet_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(BraveWalletUtilsUnitTest, ToHex) {
  ASSERT_EQ(ToHex(""), "0x0");
  ASSERT_EQ(ToHex("hello world"), "0x68656c6c6f20776f726c64");
}

TEST(BraveWalletUtilsUnitTest, KeccakHash) {
  ASSERT_EQ(
      KeccakHash(""),
      "0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");
  ASSERT_EQ(
      KeccakHash("hello world"),
      "0x47173285a8d7341e5e972fc677286384f802f8ef42a5ec5f03bbfa254cb01fad");
}

TEST(BraveWalletUtilsUnitTest, GetFunctionHash) {
  ASSERT_EQ(GetFunctionHash("approve(address,uint256)"), "0x095ea7b3");
  ASSERT_EQ(GetFunctionHash("balanceOf(address)"), "0x70a08231");
}

}  // namespace brave_wallet
