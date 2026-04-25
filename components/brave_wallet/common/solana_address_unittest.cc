/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/solana_address.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

std::vector<uint8_t> GetAccountBytes() {
  return std::vector<uint8_t>({34,  208, 53,  54,  75,  46,  112, 55,
                               123, 15,  232, 9,   45,  178, 252, 196,
                               62,  64,  169, 213, 66,  87,  192, 16,
                               152, 108, 254, 148, 183, 39,  51,  192});
}

std::string GetAccountString() {
  return "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw";
}

}  // namespace

TEST(SolanaAddressUnitTest, IsValid) {
  EXPECT_FALSE(SolanaAddress().IsValid());
  EXPECT_TRUE(SolanaAddress::FromBytes(GetAccountBytes()));
}

TEST(SolanaAddressUnitTest, FromBytes) {
  EXPECT_FALSE(SolanaAddress::FromBytes(std::vector<uint8_t>()));

  auto address = SolanaAddress::FromBytes(GetAccountBytes());
  EXPECT_TRUE(address);
  EXPECT_TRUE(address->IsValid());
  EXPECT_EQ(GetAccountString(), address->ToBase58());
}

TEST(SolanaAddressUnitTest, FromBase58) {
  EXPECT_FALSE(SolanaAddress::FromBase58(""));
  EXPECT_FALSE(SolanaAddress::FromBase58("123"));

  auto address = SolanaAddress::FromBase58(GetAccountString());
  EXPECT_TRUE(address);
  EXPECT_TRUE(address->IsValid());
  EXPECT_EQ(GetAccountBytes(), address->bytes());
}

TEST(SolanaAddressUnitTest, ZeroAddress) {
  EXPECT_EQ(SolanaAddress::ZeroAddress().ToBase58(),
            "11111111111111111111111111111111");
}

}  // namespace brave_wallet
