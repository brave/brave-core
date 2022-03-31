/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/hash_utils.h"

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(HashUtilsUnitTest, KeccakHash) {
  ASSERT_EQ(
      KeccakHash(""),
      "0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");
  ASSERT_EQ(
      KeccakHash("hello world"),
      "0x47173285a8d7341e5e972fc677286384f802f8ef42a5ec5f03bbfa254cb01fad");
}

TEST(HashUtilsUnitTest, GetFunctionHash) {
  ASSERT_EQ(GetFunctionHash("transfer(address,uint256)"), "0xa9059cbb");
  ASSERT_EQ(GetFunctionHash("approve(address,uint256)"), "0x095ea7b3");
  ASSERT_EQ(GetFunctionHash("balanceOf(address)"), "0x70a08231");
}

TEST(HashUtilsUnitTest, Namehash) {
  EXPECT_EQ(
      Namehash(""),
      "0x0000000000000000000000000000000000000000000000000000000000000000");
  EXPECT_EQ(
      Namehash("eth"),
      "0x93cdeb708b7545dc668eb9280176169d1c33cfd8ed6f04690a0bcc88a93fc4ae");
  EXPECT_EQ(
      Namehash("foo.eth"),
      "0xde9b09fd7c5f901e23a3f19fecc54828e9c848539801e86591bd9801b019f84f");
  EXPECT_EQ(
      Namehash("."),
      "0x0000000000000000000000000000000000000000000000000000000000000000");
  EXPECT_EQ(
      Namehash("crypto"),
      "0x0f4a10a4f46c288cea365fcf45cccf0e9d901b945b9829ccdb54c10dc3cb7a6f");
  EXPECT_EQ(
      Namehash("example.crypto"),
      "0xd584c5509c6788ad9d9491be8ba8b4422d05caf62674a98fbf8a9988eeadfb7e");
  EXPECT_EQ(
      Namehash("www.example.crypto"),
      "0x3ae54ac25ccd63401d817b6d79a4a56ae7f79a332fe77a98fa0c9d10adf9b2a1");
  EXPECT_EQ(
      Namehash("a.b.c.crypto"),
      "0x353ea3e0449067382e0ea7934767470170dcfa9c49b1be0fe708adc4b1f9cf13");
  EXPECT_EQ(
      Namehash("brave.crypto"),
      "0x77252571a99feee8f5e6b2f0c8b705407d395adc00b3c8ebcc7c19b2ea850013");
}

}  // namespace brave_wallet
