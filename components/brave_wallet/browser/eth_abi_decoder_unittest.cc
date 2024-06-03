/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_abi_decoder.h"

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/eth_abi_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(EthABIDecoderTest, ABIDecodeAddress) {
  std::vector<uint8_t> data;

  // OK: 32-bytes well-formed address
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460f",
      &data));
  auto decoded = ABIDecode(
      eth_abi::Tuple().AddTupleType(eth_abi::Address()).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(),
            base::Value("0xbfb30a082f650c2a15d0632f0e87be4f8e64460f"));

  // KO: insufficient address length
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64", &data));
  EXPECT_FALSE(ABIDecode(
      eth_abi::Tuple().AddTupleType(eth_abi::Address()).build(), data));

  // KO: invalid address
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x00000000000000000000e6004226bc1f1ba37e5c2c4689693b94b863cd58", &data));
  EXPECT_FALSE(ABIDecode(
      eth_abi::Tuple().AddTupleType(eth_abi::Address()).build(), data));
}

TEST(EthABIDecoderTest, ABIDecodeUint8) {
  std::vector<uint8_t> data;

  // OK: 32-bytes well-formed uint8
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x00000000000000000000000000000000000000000000000000000000000000ff",
      &data));
  auto decoded =
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Uint(8)).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value("0xff"));

  // OK: extra uint8 length
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x00000000000000000000000000000000000000000000000000000000000000ff"
      "ff",
      &data));
  decoded =
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Uint(8)).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value("0xff"));

  // KO: insufficient uint8 length
  ASSERT_TRUE(PrefixedHexStringToBytes("0xff", &data));
  EXPECT_FALSE(
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Uint(8)).build(), data));

  // KO: outside range of uint8
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x0000000000000000000000000000000000000000000000000000000000000100",
      &data));
  EXPECT_FALSE(
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Uint(8)).build(), data));
}

TEST(EthABIDecoderTest, ABIDecodeUint16) {
  std::vector<uint8_t> data;

  // OK: 32-bytes well-formed uint16
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x0000000000000000000000000000000000000000000000000000000000000fff",
      &data));
  auto decoded =
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Uint(16)).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value("0xfff"));

  // OK: extra uint16 length
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x0000000000000000000000000000000000000000000000000000000000000fff"
      "ff",
      &data));
  decoded =
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Uint(16)).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value("0xfff"));

  // KO: insufficient uint16 length
  ASSERT_TRUE(PrefixedHexStringToBytes("0xff", &data));
  EXPECT_FALSE(ABIDecode(
      eth_abi::Tuple().AddTupleType(eth_abi::Uint(16)).build(), data));

  // KO: outside range of uint16
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x0000000000000000000000000000000000000000000000000000000000010000",
      &data));
  EXPECT_FALSE(ABIDecode(
      eth_abi::Tuple().AddTupleType(eth_abi::Uint(16)).build(), data));
}

TEST(EthABIDecoderTest, ABIDecodeUint32) {
  std::vector<uint8_t> data;

  // OK: 32-bytes well-formed uint32
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x00000000000000000000000000000000000000000000000000000000ffffffff",
      &data));
  auto decoded =
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Uint(32)).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value("0xffffffff"));

  // OK: extra uint32 length
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x00000000000000000000000000000000000000000000000000000000ffffffff"
      "ff",
      &data));
  decoded =
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Uint(32)).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value("0xffffffff"));

  // KO: insufficient uint16 length
  ASSERT_TRUE(PrefixedHexStringToBytes("0xff", &data));
  EXPECT_FALSE(ABIDecode(
      eth_abi::Tuple().AddTupleType(eth_abi::Uint(32)).build(), data));

  // KO: outside range of uint32
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x0000000000000000000000000000000000000000000000000000000100000000",
      &data));
  EXPECT_FALSE(ABIDecode(
      eth_abi::Tuple().AddTupleType(eth_abi::Uint(32)).build(), data));
}

TEST(EthABIDecoderTest, ABIDecodeUint64) {
  std::vector<uint8_t> data;

  // OK: 32-bytes well-formed uint64
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x000000000000000000000000000000000000000000000000ffffffffffffffff",
      &data));
  auto decoded =
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Uint(64)).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value("0xffffffffffffffff"));

  // OK: extra uint64 length
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x000000000000000000000000000000000000000000000000ffffffffffffffff"
      "ff",
      &data));
  decoded =
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Uint(64)).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value("0xffffffffffffffff"));

  // KO: insufficient uint16 length
  ASSERT_TRUE(PrefixedHexStringToBytes("0xff", &data));
  EXPECT_FALSE(ABIDecode(
      eth_abi::Tuple().AddTupleType(eth_abi::Uint(64)).build(), data));

  // KO: outside range of uint64
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x0000000000000000000000000000000000000000000000010000000000000000",
      &data));
  EXPECT_FALSE(ABIDecode(
      eth_abi::Tuple().AddTupleType(eth_abi::Uint(64)).build(), data));
}

TEST(EthABIDecoderTest, ABIDecodeUint128) {
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  std::vector<uint8_t> data;

  // OK: 32-bytes well-formed uint128
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x00000000000000000000000000000000ffffffffffffffffffffffffffffffff",
      &data));
  auto decoded = ABIDecode(
      eth_abi::Tuple().AddTupleType(eth_abi::Uint(128)).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value("0xffffffffffffffffffffffffffffffff"));

  // OK: extra uint128 length
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x00000000000000000000000000000000ffffffffffffffffffffffffffffffff"
      "ff",
      &data));
  decoded = ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Uint(128)).build(),
                      data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value("0xffffffffffffffffffffffffffffffff"));

  // KO: insufficient uint128 length
  ASSERT_TRUE(PrefixedHexStringToBytes("0xff", &data));
  EXPECT_FALSE(ABIDecode(
      eth_abi::Tuple().AddTupleType(eth_abi::Uint(128)).build(), data));

  // KO: outside range of uint128
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x0000000000000000000000000000000100000000000000000000000000000000",
      &data));
  EXPECT_FALSE(ABIDecode(
      eth_abi::Tuple().AddTupleType(eth_abi::Uint(128)).build(), data));
}

TEST(EthABIDecoderTest, ABIDecodeUint256) {
  std::vector<uint8_t> data;

  // OK: 32-bytes well-formed uint256
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x00000000000000000000000000000000000000000000000000000000000000ff",
      &data));
  auto decoded = ABIDecode(
      eth_abi::Tuple().AddTupleType(eth_abi::Uint(256)).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value("0xff"));

  // KO: insufficient uint256 length
  ASSERT_TRUE(PrefixedHexStringToBytes("0xff", &data));
  ASSERT_FALSE(ABIDecode(
      eth_abi::Tuple().AddTupleType(eth_abi::Uint(256)).build(), data));

  // OK: extra uint256 length
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x00000000000000000000000000000000000000000000000000000000000000ff"
      "ff",
      &data));
  decoded = ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Uint(256)).build(),
                      data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value("0xff"));
}

TEST(EthABIDecoderTest, ABIDecodeBool) {
  std::vector<uint8_t> data;

  // OK: false
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x0000000000000000000000000000000000000000000000000000000000000000",
      &data));
  auto decoded =
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Bool()).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value(false));

  // OK: true
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x0000000000000000000000000000000000000000000000000000000000000001",
      &data));
  decoded =
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Bool()).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value(true));

  // KO: insufficient bool length
  ASSERT_TRUE(PrefixedHexStringToBytes("0x0", &data));
  EXPECT_FALSE(
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Bool()).build(), data));

  // OK: extra bool length
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x0000000000000000000000000000000000000000000000000000000000000000"
      "00",
      &data));
  decoded =
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Bool()).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value(false));

  // KO: invalid bool value
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x0000000000000000000000000000000000000000000000000000000000000fff",
      &data));
  EXPECT_FALSE(
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Bool()).build(), data));
}

TEST(EthABIDecoderTest, ABIDecodeArray) {
  std::vector<uint8_t> data;

  // OK: 2-element address[]
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000002"
      "00000000000000000000000000000000000000000000000000000000000000ff"
      "0000000000000000000000000000000000000000000000000000000000000fff",
      &data));
  auto decoded = ABIDecode(
      eth_abi::Tuple()
          .AddTupleType(
              eth_abi::Array().SetArrayType(eth_abi::Address()).build())
          .build(),
      data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_TRUE(decoded->back().is_list());
  EXPECT_EQ(decoded->back().GetList().size(), 2UL);
  EXPECT_EQ(decoded->back().GetList()[0],
            base::Value("0x00000000000000000000000000000000000000ff"));
  EXPECT_EQ(decoded->back().GetList()[1],
            base::Value("0x0000000000000000000000000000000000000fff"));

  // OK: empty address[]
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000000",
      &data));
  decoded = ABIDecode(
      eth_abi::Tuple()
          .AddTupleType(
              eth_abi::Array().SetArrayType(eth_abi::Address()).build())
          .build(),
      data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_TRUE(decoded->back().is_list());
  EXPECT_EQ(decoded->back().GetList().size(), 0UL);

  // OK: valid address[] with extra tail calldata
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000002"
      "00000000000000000000000000000000000000000000000000000000000000ff"
      "0000000000000000000000000000000000000000000000000000000000000fff"
      "ffff",
      &data));
  decoded = ABIDecode(
      eth_abi::Tuple()
          .AddTupleType(
              eth_abi::Array().SetArrayType(eth_abi::Address()).build())
          .build(),
      data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_TRUE(decoded->back().is_list());
  EXPECT_EQ(decoded->back().GetList().size(), 2UL);
  EXPECT_EQ(decoded->back().GetList()[0],
            base::Value("0x00000000000000000000000000000000000000ff"));
  EXPECT_EQ(decoded->back().GetList()[1],
            base::Value("0x0000000000000000000000000000000000000fff"));

  // KO: invalid offset (out of calldata range) for address[]
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x00000000000000000000000000000000000000000000000000000000000000ff",
      &data));
  EXPECT_FALSE(ABIDecode(
      eth_abi::Tuple()
          .AddTupleType(
              eth_abi::Array().SetArrayType(eth_abi::Address()).build())
          .build(),
      data));

  // KO: invalid offset (number too large) for address[]
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
      "0000000000000000000000000000000000000000000000000000000000000000",
      &data));
  EXPECT_FALSE(ABIDecode(
      eth_abi::Tuple()
          .AddTupleType(
              eth_abi::Array().SetArrayType(eth_abi::Address()).build())
          .build(),
      data));

  // KO: invalid address[] length (insufficient number of elements)
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "00000000000000000000000000000000000000000000000000000000000000ff"
      "0000000000000000000000000000000000000000000000000000000000000000",
      &data));
  EXPECT_FALSE(ABIDecode(
      eth_abi::Tuple()
          .AddTupleType(
              eth_abi::Array().SetArrayType(eth_abi::Address()).build())
          .build(),
      data));

  // KO: invalid address[] length (number too large)
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
      &data));
  EXPECT_FALSE(ABIDecode(
      eth_abi::Tuple()
          .AddTupleType(
              eth_abi::Array().SetArrayType(eth_abi::Address()).build())
          .build(),
      data));

  // KO: invalid address[] contents
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "00000000000000000000000000000000000000000000000000000000000000ff"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "ffff",
      &data));
  EXPECT_FALSE(ABIDecode(
      eth_abi::Tuple()
          .AddTupleType(
              eth_abi::Array().SetArrayType(eth_abi::Address()).build())
          .build(),
      data));

  // OK: 2-element uint[]
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000002"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "0000000000000000000000000000000000000000000000000000000000000003",
      &data));
  decoded = ABIDecode(
      eth_abi::Tuple()
          .AddTupleType(eth_abi::Array().SetArrayType(eth_abi::Uint()).build())
          .build(),
      data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_TRUE(decoded->back().is_list());
  EXPECT_EQ(decoded->back().GetList().size(), 2UL);
  EXPECT_EQ(decoded->back().GetList()[0], base::Value("0x1"));
  EXPECT_EQ(decoded->back().GetList()[1], base::Value("0x3"));

  // OK: uint[2]
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "0000000000000000000000000000000000000000000000000000000000000003",
      &data));
  decoded = ABIDecode(
      eth_abi::Tuple()
          .AddTupleType(eth_abi::Array(2).SetArrayType(eth_abi::Uint()).build())
          .build(),
      data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_TRUE(decoded->back().is_list());
  EXPECT_EQ(decoded->back().GetList().size(), 2UL);
  EXPECT_EQ(decoded->back().GetList()[0], base::Value("0x1"));
  EXPECT_EQ(decoded->back().GetList()[1], base::Value("0x3"));

  // OK: string[2]
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000000000000000000000000000000000000000000080"
      "0000000000000000000000000000000000000000000000000000000000000005"
      "68656c6c6f000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000004"
      "7765623300000000000000000000000000000000000000000000000000000000",
      &data));
  decoded = ABIDecode(
      eth_abi::Tuple()
          .AddTupleType(
              eth_abi::Array(2).SetArrayType(eth_abi::String()).build())
          .build(),
      data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_TRUE(decoded->back().is_list());
  EXPECT_EQ(decoded->back().GetList().size(), 2UL);
  EXPECT_EQ(decoded->back().GetList()[0], base::Value("hello"));
  EXPECT_EQ(decoded->back().GetList()[1], base::Value("web3"));

  // OK: string[]
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000002"
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000000000000000000000000000000000000000000080"
      "0000000000000000000000000000000000000000000000000000000000000005"
      "68656c6c6f000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000004"
      "7765623300000000000000000000000000000000000000000000000000000000",
      &data));
  decoded = ABIDecode(
      eth_abi::Tuple()
          .AddTupleType(
              eth_abi::Array().SetArrayType(eth_abi::String()).build())
          .build(),
      data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_TRUE(decoded->back().is_list());
  EXPECT_EQ(decoded->back().GetList().size(), 2UL);
  EXPECT_EQ(decoded->back().GetList()[0], base::Value("hello"));
  EXPECT_EQ(decoded->back().GetList()[1], base::Value("web3"));
}

TEST(EthABIDecoderTest, ABIDecodeBytes) {
  std::vector<uint8_t> data;

  // OK: valid 2 bytes
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000002"
      "ffff",
      &data));
  auto decoded =
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Bytes()).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value("0xffff"));

  // OK: valid 2 bytes with extra tail calldata
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000002"
      "ffff"
      "ffffff",  // extraneous tail data,
      &data));
  decoded =
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Bytes()).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value("0xffff"));

  // OK: empty bytes
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000000",
      &data));
  decoded =
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Bytes()).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value("0x"));

  // KO: invalid offset (out of range)
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x00000000000000000000000000000000000000000000000000000000000000ff",
      &data));
  EXPECT_FALSE(
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Bytes()).build(), data));

  // KO: invalid offset (number too large)
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
      "0000000000000000000000000000000000000000000000000000000000000000",
      &data));
  EXPECT_FALSE(
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Bytes()).build(), data));

  // KO: invalid bytes length
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "00000000000000000000000000000000000000000000000000000000000000ff"
      "ff",
      &data));
  EXPECT_FALSE(
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Bytes()).build(), data));

  // KO: invalid bytes length (number too large)
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
      &data));
  EXPECT_FALSE(
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Bytes()).build(), data));

  // OK: valid bytes5
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000001010000000000000000000000000000000000000000000000000000000",
      &data));
  decoded =
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Bytes(5)).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value("0x0000001010"));

  // OK: valid bytes2
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0102000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000002",
      &data));
  decoded =
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::Bytes(2)).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value("0x0102"));
}

TEST(ETHABIDecoderTest, ABIDecodeString) {
  std::vector<uint8_t> data;

  // OK: valid string
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000005"
      "6272617665000000000000000000000000000000000000000000000000000000",
      &data));
  auto decoded =
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::String()).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value("brave"));

  // OK: extra long string
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "000000000000000000000000000000000000000000000000000000000000008a"
      "65787472616161616161616161616c6f6f6f6f6f6f6f6f6f6f6f6f6f6f6f6f6f"
      "6f6f6e6e6e6e6e6e6e6e6e6e6e6e6e6e6e6e6e6e6e6e6e6e6e6e6e6e6e6e6e6e"
      "6e6e6e6767676767676767676767676767676767676767676767737472727272"
      "7272727272727272727272727272727272727272696969696969696969696969"
      "6e6e6e6e6e6e6e6e6e6700000000000000000000000000000000000000000000",
      &data));
  decoded =
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::String()).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(),
            base::Value("extraaaaaaaaaalooooooooooooooooooonnnnnnnnnnnnnnnnnnnn"
                        "nnnnnnnnnnnnngggggggggggggggggggggggstrrrrrrrrrrrrrrrr"
                        "rrrrrrrriiiiiiiiiiiinnnnnnnnng"));

  // OK: string with funny characters
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "000000000000000000000000000000000000000000000000000000000000000a"
      "c5a1c48d7ce282ac2d2100000000000000000000000000000000000000000000",
      &data));
  decoded =
      ABIDecode(eth_abi::Tuple().AddTupleType(eth_abi::String()).build(), data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_EQ(decoded->back(), base::Value("šč|€-!"));
}

TEST(EthABIDecoderTest, ABIDecodeTuple) {
  std::vector<uint8_t> data;

  // OK: (uint8, bool)
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000045"
      "0000000000000000000000000000000000000000000000000000000000000001",
      &data));
  auto decoded = ABIDecode(eth_abi::Tuple()
                               .AddTupleType(eth_abi::Tuple()
                                                 .AddTupleType(eth_abi::Uint(8))
                                                 .AddTupleType(eth_abi::Bool())
                                                 .build())
                               .build(),
                           data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_TRUE(decoded->back().is_list());
  ASSERT_EQ(decoded->back().GetList().size(), 2UL);
  EXPECT_EQ(decoded->back().GetList()[0], base::Value("0x45"));
  EXPECT_EQ(decoded->back().GetList()[1], base::Value(true));

  // OK: (bool, (uint8, bool))
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "0000000000000000000000000000000000000000000000000000000000000045"
      "0000000000000000000000000000000000000000000000000000000000000001",
      &data));
  decoded = ABIDecode(
      eth_abi::Tuple()
          .AddTupleType(eth_abi::Tuple()
                            .AddTupleType(eth_abi::Bool())
                            .AddTupleType(eth_abi::Tuple()
                                              .AddTupleType(eth_abi::Uint(8))
                                              .AddTupleType(eth_abi::Bool())
                                              .build())
                            .build())
          .build(),
      data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_TRUE(decoded->back().is_list());
  ASSERT_EQ(decoded->back().GetList().size(), 2UL);
  EXPECT_EQ(decoded->back().GetList()[0], base::Value(true));
  EXPECT_TRUE(decoded->back().GetList()[1].is_list());
  ASSERT_EQ(decoded->back().GetList()[1].GetList().size(), 2UL);
  EXPECT_EQ(decoded->back().GetList()[1].GetList()[0], base::Value("0x45"));
  EXPECT_EQ(decoded->back().GetList()[1].GetList()[1], base::Value(true));

  // OK: (string, bool, (bool, string))
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000060"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "0000000000000000000000000000000000000000000000000000000000000011"
      "6d6172696e313233313233313233313233000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "0000000000000000000000000000000000000000000000000000000000000040"
      "000000000000000000000000000000000000000000000000000000000000003c"
      "776562336a7374657374696e676c6f6e67737472696969696969696969696969"
      "69696969696969696969696969696969696969696969696969696e6700000000",
      &data));
  decoded = ABIDecode(
      eth_abi::Tuple()
          .AddTupleType(eth_abi::Tuple()
                            .AddTupleType(eth_abi::String())
                            .AddTupleType(eth_abi::Bool())
                            .AddTupleType(eth_abi::Tuple()
                                              .AddTupleType(eth_abi::Bool())
                                              .AddTupleType(eth_abi::String())
                                              .build())
                            .build())
          .build(),
      data);
  ASSERT_NE(decoded, std::nullopt);
  ASSERT_EQ(decoded->size(), 1UL);
  EXPECT_TRUE(decoded->back().is_list());
  ASSERT_EQ(decoded->back().GetList().size(), 3UL);
  EXPECT_EQ(decoded->back().GetList()[0], base::Value("marin123123123123"));
  EXPECT_EQ(decoded->back().GetList()[1], base::Value(true));
  EXPECT_TRUE(decoded->back().GetList()[2].is_list());
  ASSERT_EQ(decoded->back().GetList()[2].GetList().size(), 2UL);
  EXPECT_EQ(decoded->back().GetList()[2].GetList()[0], base::Value(true));
  EXPECT_EQ(
      decoded->back().GetList()[2].GetList()[1],
      base::Value(
          "web3jstestinglongstriiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiing"));
}

TEST(EthABIDecoderTest, UniswapEncodedPathDecodeValid) {
  // Single-hop swap: WETH → STG
  std::optional<std::vector<std::string>> path;
  path = UniswapEncodedPathDecode(
      "0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2"  // WETH
      "002710"                                      // POOL FEE (10000)
      "af5191b0de278c7286d6c7cc6ab6bb8a73ba2cd6");  // STG
  ASSERT_NE(path, std::nullopt);
  EXPECT_EQ(path->size(), 2UL);
  EXPECT_EQ(path->at(0), "0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2");
  EXPECT_EQ(path->at(1), "0xaf5191b0de278c7286d6c7cc6ab6bb8a73ba2cd6");

  // Multi-hop swap: RSS3 → USDC → WETH
  path = UniswapEncodedPathDecode(
      "0xc98d64da73a6616c42117b582e832812e7b8d57f"  // RSS3
      "000bb8"                                      // POOL FEE (3000)
      "a0b86991c6218b36c1d19d4a2e9eb0ce3606eb48"    // USDC
      "0001f4"                                      // POOL FEE (500)
      "c02aaa39b223fe8d0a0e5c4f27ead9083c756cc2");  // WETH
  ASSERT_NE(path, std::nullopt);
  ASSERT_EQ(path->size(), 3UL);
  EXPECT_EQ(path->at(0), "0xc98d64da73a6616c42117b582e832812e7b8d57f");
  EXPECT_EQ(path->at(1), "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48");
  EXPECT_EQ(path->at(2), "0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2");
}

TEST(EthABIDecoderTest, UniswapEncodedPathDecodeInvalid) {
  // Empty string.
  EXPECT_FALSE(UniswapEncodedPathDecode(""));

  // Missing hops.
  EXPECT_FALSE(UniswapEncodedPathDecode("0x"));

  // Missing source hop.
  EXPECT_FALSE(UniswapEncodedPathDecode(
      "0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2"  // WETH
      "002710"));                                   // POOL FEE

  // Missing destination hop.
  EXPECT_FALSE(UniswapEncodedPathDecode(
      "0x002710"                                     // POOL FEE
      "af5191b0de278c7286d6c7cc6ab6bb8a73ba2cd6"));  // STG

  // Missing POOL FEE
  EXPECT_FALSE(UniswapEncodedPathDecode(
      "0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2"   // WETH
      "af5191b0de278c7286d6c7cc6ab6bb8a73ba2cd6"));  // STG

  // Extraneous data
  EXPECT_FALSE(UniswapEncodedPathDecode(
      "0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2"  // WETH
      "002710"                                      // POOL FEE
      "af5191b0de278c7286d6c7cc6ab6bb8a73ba2cd6"    // STG
      "deadbeef"));                                 // Bogus data
}

}  // namespace brave_wallet
