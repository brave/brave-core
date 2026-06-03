/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/solana_address.h"

#include "base/containers/span_reader.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

constexpr std::array<uint8_t, kSolanaPubkeySize> kAccountBytes = {
    34, 208, 53,  54,  75, 46, 112, 55, 123, 15,  232, 9,   45,  178, 252, 196,
    62, 64,  169, 213, 66, 87, 192, 16, 152, 108, 254, 148, 183, 39,  51,  192,
};

std::string GetAccountString() {
  return "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw";
}

}  // namespace

TEST(SolanaAddressUnitTest, FromBytes) {
  EXPECT_FALSE(SolanaAddress::FromBytes(std::vector<uint8_t>()));

  auto address = SolanaAddress::FromBytes(kAccountBytes);
  EXPECT_TRUE(address);
  EXPECT_EQ(GetAccountString(), address->ToBase58());
}

TEST(SolanaAddressUnitTest, ReadFrom) {
  base::SpanReader<const uint8_t> reader;
  EXPECT_FALSE(SolanaAddress::ReadFrom(reader));

  reader = base::SpanReader(base::span(kAccountBytes));
  auto address = SolanaAddress::ReadFrom(reader);
  EXPECT_TRUE(address);
  EXPECT_EQ(GetAccountString(), address->ToBase58());
  EXPECT_EQ(0u, reader.remaining());
}

TEST(SolanaAddressUnitTest, FromBase58) {
  EXPECT_FALSE(SolanaAddress::FromBase58(""));
  EXPECT_FALSE(SolanaAddress::FromBase58("123"));

  auto address = SolanaAddress::FromBase58(GetAccountString());
  EXPECT_TRUE(address);
  EXPECT_EQ(base::span(kAccountBytes), address->bytes());
}

TEST(SolanaAddressUnitTest, ZeroAddress) {
  EXPECT_EQ(SolanaAddress::ZeroAddress().ToBase58(),
            "11111111111111111111111111111111");
}

}  // namespace brave_wallet
