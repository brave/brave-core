/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/btc_like_serializer_stream.h"

#include <memory>
#include <string>

#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(BtcLikeSerializerStreamTest, Push8AsLE) {
  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  stream.Push8AsLE(0xab);
  EXPECT_EQ(base::HexEncode(data), "AB");
  stream.Push8AsLE(0x12);
  EXPECT_EQ(base::HexEncode(data), "AB12");

  EXPECT_EQ(stream.serialized_bytes(), 2u);
}

TEST(BtcLikeSerializerStreamTest, Push16AsLE) {
  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  stream.Push16AsLE(0xab);
  EXPECT_EQ(base::HexEncode(data), "AB00");
  stream.Push16AsLE(0x1234);
  EXPECT_EQ(base::HexEncode(data), "AB003412");

  EXPECT_EQ(stream.serialized_bytes(), 4u);
}

TEST(BtcLikeSerializerStreamTest, Push32AsLE) {
  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  stream.Push32AsLE(0xabcd);
  EXPECT_EQ(base::HexEncode(data), "CDAB0000");
  stream.Push32AsLE(0x12345678);
  EXPECT_EQ(base::HexEncode(data), "CDAB000078563412");

  EXPECT_EQ(stream.serialized_bytes(), 8u);
}

TEST(BtcLikeSerializerStreamTest, Push64AsLE) {
  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  stream.Push64AsLE(0xabcd);
  EXPECT_EQ(base::HexEncode(data), "CDAB000000000000");
  stream.Push64AsLE(0x1234567890abcdef);
  EXPECT_EQ(base::HexEncode(data), "CDAB000000000000EFCDAB9078563412");

  EXPECT_EQ(stream.serialized_bytes(), 16u);
}

TEST(BtcLikeSerializerStreamTest, PushVarInt) {
  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  stream.PushVarInt(0xab);
  EXPECT_EQ(base::HexEncode(data), "AB");
  stream.PushVarInt(0xabcd);
  EXPECT_EQ(base::HexEncode(data), "ABFDCDAB");
  stream.PushVarInt(0xabcdef01);
  EXPECT_EQ(base::HexEncode(data), "ABFDCDABFE01EFCDAB");
  stream.PushVarInt(0xabcdef0123456789);
  EXPECT_EQ(base::HexEncode(data), "ABFDCDABFE01EFCDABFF8967452301EFCDAB");

  EXPECT_EQ(stream.serialized_bytes(), 18u);
}

TEST(BtcLikeSerializerStreamTest, PushSizeAndBytes) {
  {
    std::vector<uint8_t> bytes(10, 0xab);
    std::vector<uint8_t> data;
    BtcLikeSerializerStream stream(&data);
    stream.PushSizeAndBytes(bytes);
    EXPECT_EQ(data.size(), 1u + 10u);
    EXPECT_EQ(base::HexEncode(base::make_span(data).first<1>()), "0A");
    EXPECT_TRUE(base::ranges::all_of(base::make_span(data).last<10>(),
                                     [](auto c) { return c == 0xab; }));
    EXPECT_EQ(stream.serialized_bytes(), 11u);
  }

  {
    std::vector<uint8_t> bytes(300, 0xcd);
    std::vector<uint8_t> data;
    BtcLikeSerializerStream stream(&data);
    stream.PushSizeAndBytes(bytes);
    EXPECT_EQ(data.size(), 3u + 300u);
    EXPECT_EQ(base::HexEncode(base::make_span(data).first<3>()), "FD2C01");
    EXPECT_TRUE(base::ranges::all_of(base::make_span(data).last<300>(),
                                     [](auto c) { return c == 0xcd; }));
    EXPECT_EQ(stream.serialized_bytes(), 303u);
  }

  {
    std::vector<uint8_t> bytes(0x10000, 0xef);
    std::vector<uint8_t> data;
    BtcLikeSerializerStream stream(&data);
    stream.PushSizeAndBytes(bytes);
    EXPECT_EQ(data.size(), 5u + 0x10000);
    EXPECT_EQ(base::HexEncode(base::make_span(data).first<5>()), "FE00000100");
    EXPECT_TRUE(base::ranges::all_of(base::make_span(data).last<0x10000>(),
                                     [](auto c) { return c == 0xef; }));
    EXPECT_EQ(stream.serialized_bytes(), 65541u);
  }
}

TEST(BtcLikeSerializerStreamTest, PushBytes) {
  std::vector<uint8_t> bytes({0x01, 0x02, 0xab, 0xcd, 0xef});
  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  stream.PushBytes(bytes);
  EXPECT_EQ(base::HexEncode(data), "0102ABCDEF");

  EXPECT_EQ(stream.serialized_bytes(), 5u);
}

TEST(BtcLikeSerializerStreamTest, PushBytesReversed) {
  std::vector<uint8_t> bytes({0x01, 0x02, 0xab, 0xcd, 0xef});
  std::vector<uint8_t> data;
  BtcLikeSerializerStream stream(&data);
  stream.PushBytesReversed(bytes);
  EXPECT_EQ(base::HexEncode(data), "EFCDAB0201");

  EXPECT_EQ(stream.serialized_bytes(), 5u);
}

TEST(BtcLikeSerializerStreamTest, NoVectorInCtor) {
  std::vector<uint8_t> bytes({0x01, 0x02, 0xab, 0xcd, 0xef});

  BtcLikeSerializerStream stream(nullptr);

  stream.Push8AsLE(0xab);
  EXPECT_EQ(stream.serialized_bytes(), 1u);

  stream.Push16AsLE(0xab);
  EXPECT_EQ(stream.serialized_bytes(), 3u);

  stream.Push32AsLE(0x12345678);
  EXPECT_EQ(stream.serialized_bytes(), 7u);

  stream.Push64AsLE(0xabcd);
  EXPECT_EQ(stream.serialized_bytes(), 15u);

  stream.PushBytes(bytes);
  EXPECT_EQ(stream.serialized_bytes(), 20u);

  stream.PushBytesReversed(bytes);
  EXPECT_EQ(stream.serialized_bytes(), 25u);

  stream.PushSizeAndBytes(bytes);
  EXPECT_EQ(stream.serialized_bytes(), 31u);

  stream.PushVarInt(0xabcdef01);
  EXPECT_EQ(stream.serialized_bytes(), 36u);
}

}  // namespace brave_wallet
