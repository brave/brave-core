/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/btc_like_serializer_stream.h"

#include <algorithm>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(BtcLikeSerializerStreamTest, Push8AsLE) {
  BtcLikeSerializerStream stream;
  stream.Push8(uint8_t{0xab});
  EXPECT_EQ(base::HexEncode(stream.data()), "AB");
  stream.Push8(uint8_t{0x12});
  EXPECT_EQ(base::HexEncode(stream.data()), "AB12");

  EXPECT_EQ(stream.data().size(), 2u);
}

TEST(BtcLikeSerializerStreamTest, Push16AsLE) {
  BtcLikeSerializerStream stream;
  stream.Push16(uint16_t{0xab});
  EXPECT_EQ(base::HexEncode(stream.data()), "AB00");
  stream.Push16(uint16_t{0x1234});
  EXPECT_EQ(base::HexEncode(stream.data()), "AB003412");

  EXPECT_EQ(stream.data().size(), 4u);
}

TEST(BtcLikeSerializerStreamTest, Push32AsLE) {
  BtcLikeSerializerStream stream;
  stream.Push32(uint32_t{0xabcd});
  EXPECT_EQ(base::HexEncode(stream.data()), "CDAB0000");
  stream.Push32(uint32_t{0x12345678});
  EXPECT_EQ(base::HexEncode(stream.data()), "CDAB000078563412");

  EXPECT_EQ(stream.data().size(), 8u);
}

TEST(BtcLikeSerializerStreamTest, Push64AsLE) {
  BtcLikeSerializerStream stream;
  stream.Push64(uint64_t{0xabcd});
  EXPECT_EQ(base::HexEncode(stream.data()), "CDAB000000000000");
  stream.Push64(uint64_t{0x1234567890abcdef});
  EXPECT_EQ(base::HexEncode(stream.data()), "CDAB000000000000EFCDAB9078563412");

  EXPECT_EQ(stream.data().size(), 16u);
}

TEST(BtcLikeSerializerStreamTest, PushCompactSize) {
  BtcLikeSerializerStream stream;
  stream.PushCompactSize(uint8_t{0xab});
  EXPECT_EQ(base::HexEncode(stream.data()), "AB");
  stream.PushCompactSize(uint64_t{0xabcd});
  EXPECT_EQ(base::HexEncode(stream.data()), "ABFDCDAB");
  stream.PushCompactSize(uint64_t{0xabcdef01});
  EXPECT_EQ(base::HexEncode(stream.data()), "ABFDCDABFE01EFCDAB");
  stream.PushCompactSize(uint64_t{0xabcdef0123456789});
  EXPECT_EQ(base::HexEncode(stream.data()),
            "ABFDCDABFE01EFCDABFF8967452301EFCDAB");

  EXPECT_EQ(stream.data().size(), 18u);
}

TEST(BtcLikeSerializerStreamTest, PushSizeAndBytes) {
  {
    std::vector<uint8_t> bytes(10, 0xab);
    BtcLikeSerializerStream stream;
    stream.PushSizeAndBytes(bytes);
    EXPECT_EQ(stream.data().size(), 1u + 10u);
    EXPECT_EQ(base::HexEncode(base::span(stream.data()).first<1>()), "0A");
    EXPECT_TRUE(std::ranges::all_of(base::span(stream.data()).last<10>(),
                                    [](auto c) { return c == 0xab; }));
    EXPECT_EQ(stream.data().size(), 11u);
  }

  {
    std::vector<uint8_t> bytes(300, 0xcd);
    BtcLikeSerializerStream stream;
    stream.PushSizeAndBytes(bytes);
    EXPECT_EQ(stream.data().size(), 3u + 300u);
    EXPECT_EQ(base::HexEncode(base::span(stream.data()).first<3>()), "FD2C01");
    EXPECT_TRUE(std::ranges::all_of(base::span(stream.data()).last<300>(),
                                    [](auto c) { return c == 0xcd; }));
    EXPECT_EQ(stream.data().size(), 303u);
  }

  {
    std::vector<uint8_t> bytes(0x10000, 0xef);
    BtcLikeSerializerStream stream;
    stream.PushSizeAndBytes(bytes);
    EXPECT_EQ(stream.data().size(), 5u + 0x10000);
    EXPECT_EQ(base::HexEncode(base::span(stream.data()).first<5>()),
              "FE00000100");
    EXPECT_TRUE(std::ranges::all_of(base::span(stream.data()).last<0x10000>(),
                                    [](auto c) { return c == 0xef; }));
    EXPECT_EQ(stream.data().size(), 65541u);
  }
}

TEST(BtcLikeSerializerStreamTest, PushBytes) {
  std::vector<uint8_t> bytes({0x01, 0x02, 0xab, 0xcd, 0xef});
  BtcLikeSerializerStream stream;
  stream.PushBytes(bytes);
  EXPECT_EQ(base::HexEncode(stream.data()), "0102ABCDEF");

  EXPECT_EQ(stream.data().size(), 5u);
}

TEST(BtcLikeSerializerStreamTest, PushBytesReversed) {
  std::vector<uint8_t> bytes({0x01, 0x02, 0xab, 0xcd, 0xef});
  BtcLikeSerializerStream stream;
  stream.PushBytesReversed(bytes);
  EXPECT_EQ(base::HexEncode(stream.data()), "EFCDAB0201");

  EXPECT_EQ(stream.data().size(), 5u);
}

TEST(BtcLikeSerializerStreamTest, NoVectorInCtor) {
  BtcLikeSerializerStream stream;

  std::vector<uint8_t> bytes({0x01, 0x02, 0xab, 0xcd, 0xef});
  stream.Push8(uint8_t{0xab});
  EXPECT_EQ(stream.data().size(), 1u);

  stream.Push16(uint16_t{0xab});
  EXPECT_EQ(stream.data().size(), 3u);

  stream.Push32(uint32_t{0x12345678});
  EXPECT_EQ(stream.data().size(), 7u);

  stream.Push64(uint64_t{0xabcd});
  EXPECT_EQ(stream.data().size(), 15u);

  stream.PushBytes(bytes);
  EXPECT_EQ(stream.data().size(), 20u);

  stream.PushBytesReversed(bytes);
  EXPECT_EQ(stream.data().size(), 25u);

  stream.PushSizeAndBytes(bytes);
  EXPECT_EQ(stream.data().size(), 31u);

  stream.PushCompactSize(uint64_t{0xabcdef01});
  EXPECT_EQ(stream.data().size(), 36u);
}

}  // namespace brave_wallet
