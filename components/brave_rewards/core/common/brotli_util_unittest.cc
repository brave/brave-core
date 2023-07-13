/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/common/brotli_util.h"

#include <array>

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter='RewardsBrotliHelpersTest.*'

namespace {

constexpr std::array<uint8_t, 68> kCompressed = {
    0x1b, 0x58, 0x00, 0xa0, 0x2c, 0x0e, 0x78, 0xd3, 0xd0, 0x95, 0x5d, 0x97,
    0x10, 0xbb, 0x17, 0x1b, 0xa1, 0xd2, 0x93, 0xaa, 0x0c, 0x2d, 0xcd, 0xc8,
    0xd8, 0x1a, 0xc4, 0x65, 0x2e, 0x6f, 0x83, 0x9c, 0xe1, 0xe9, 0xa9, 0xb0,
    0x37, 0x70, 0xc8, 0x01, 0x73, 0xbb, 0x40, 0x5e, 0x84, 0xb1, 0x57, 0x03,
    0x50, 0x6e, 0x3c, 0xa7, 0x3a, 0x72, 0x1c, 0x51, 0x4c, 0xc1, 0x13, 0xb8,
    0xfb, 0x6a, 0x6c, 0x65, 0x2a, 0xb6, 0x2a, 0x16};

constexpr char kUncompressed[] =
    "The quick brown fox jumps over the lazy dog. "
    "The quick dog jumps over the lazy brown fox.";

}  // namespace

namespace brave_rewards::internal {
namespace util {

class RewardsBrotliHelpersTest : public testing::Test {
 protected:
  static std::string GetInput() {
    return std::string(kCompressed.begin(), kCompressed.end());
  }
};

TEST_F(RewardsBrotliHelpersTest, TestDecode) {
  std::string uncompressed(kUncompressed);
  std::string s;

  EXPECT_TRUE(DecodeBrotliString(GetInput(), uncompressed.size(), &s));
  EXPECT_EQ(s, uncompressed);

  // Empty input
  EXPECT_FALSE(DecodeBrotliString("", uncompressed.size(), &s));

  // Uncompressed size not large enough
  EXPECT_FALSE(DecodeBrotliString(GetInput(), 16, &s));

  // Not Brotli
  EXPECT_FALSE(DecodeBrotliString("not brotli", 16, &s));
}

TEST_F(RewardsBrotliHelpersTest, TestDecodeWithBuffer) {
  std::string s;

  EXPECT_TRUE(DecodeBrotliStringWithBuffer(GetInput(), 16, &s));
  EXPECT_EQ(s, std::string(kUncompressed));

  // Empty input
  EXPECT_FALSE(DecodeBrotliStringWithBuffer("", 16, &s));

  // Incomplete input
  EXPECT_FALSE(DecodeBrotliStringWithBuffer(GetInput().substr(0, 32), 16, &s));

  // Not Brotli
  EXPECT_FALSE(DecodeBrotliStringWithBuffer("not brotli", 16, &s));
}

}  // namespace util
}  // namespace brave_rewards::internal
