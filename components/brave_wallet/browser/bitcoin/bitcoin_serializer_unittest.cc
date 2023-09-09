/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_serializer.h"

#include <memory>
#include <string>

#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {
namespace {
const char kTxid1[] =
    "aa388f50b725767653e150ad8990ec11a2146d75acafbe492af08213849fe2c5";
const char kTxid2[] =
    "bd1c9cfb126a519f3ee593bbbba41a0f9d55b4d267e9483673a848242bc5c2be";
const char kAddress1[] = "tb1qya3rarek59486w345v45tv6nra4fy2xxgky26x";
const char kAddress2[] = "tb1qva8clyftt2fstawn5dy0nvrfmygpzulf3lwulm";

}  // namespace

TEST(BitcoinSerializerStream, Push8AsLE) {
  std::vector<uint8_t> data;
  BitcoinSerializerStream stream(data);
  stream.Push8AsLE(0xab);
  EXPECT_EQ(base::HexEncode(data), "AB");
  stream.Push8AsLE(0x12);
  EXPECT_EQ(base::HexEncode(data), "AB12");
}

TEST(BitcoinSerializerStream, Push16AsLE) {
  std::vector<uint8_t> data;
  BitcoinSerializerStream stream(data);
  stream.Push16AsLE(0xab);
  EXPECT_EQ(base::HexEncode(data), "AB00");
  stream.Push16AsLE(0x1234);
  EXPECT_EQ(base::HexEncode(data), "AB003412");
}

TEST(BitcoinSerializerStream, Push32AsLE) {
  std::vector<uint8_t> data;
  BitcoinSerializerStream stream(data);
  stream.Push32AsLE(0xabcd);
  EXPECT_EQ(base::HexEncode(data), "CDAB0000");
  stream.Push32AsLE(0x12345678);
  EXPECT_EQ(base::HexEncode(data), "CDAB000078563412");
}

TEST(BitcoinSerializerStream, Push64AsLE) {
  std::vector<uint8_t> data;
  BitcoinSerializerStream stream(data);
  stream.Push64AsLE(0xabcd);
  EXPECT_EQ(base::HexEncode(data), "CDAB000000000000");
  stream.Push64AsLE(0x1234567890abcdef);
  EXPECT_EQ(base::HexEncode(data), "CDAB000000000000EFCDAB9078563412");
}

TEST(BitcoinSerializerStream, PushVarInt) {
  std::vector<uint8_t> data;
  BitcoinSerializerStream stream(data);
  stream.PushVarInt(0xab);
  EXPECT_EQ(base::HexEncode(data), "AB");
  stream.PushVarInt(0xabcd);
  EXPECT_EQ(base::HexEncode(data), "ABFDCDAB");
  stream.PushVarInt(0xabcdef01);
  EXPECT_EQ(base::HexEncode(data), "ABFDCDABFE01EFCDAB");
  stream.PushVarInt(0xabcdef0123456789);
  EXPECT_EQ(base::HexEncode(data), "ABFDCDABFE01EFCDABFF8967452301EFCDAB");
}

TEST(BitcoinSerializerStream, PushSizeAndBytes) {
  {
    std::vector<uint8_t> bytes(10, 0xab);
    std::vector<uint8_t> data;
    BitcoinSerializerStream stream(data);
    stream.PushSizeAndBytes(bytes);
    EXPECT_EQ(data.size(), 1u + 10u);
    EXPECT_EQ(base::HexEncode(base::make_span(data).first(1)), "0A");
    EXPECT_TRUE(base::ranges::all_of(base::make_span(data).last(10),
                                     [](auto c) { return c == 0xab; }));
  }

  {
    std::vector<uint8_t> bytes(300, 0xcd);
    std::vector<uint8_t> data;
    BitcoinSerializerStream stream(data);
    stream.PushSizeAndBytes(bytes);
    EXPECT_EQ(data.size(), 3u + 300u);
    EXPECT_EQ(base::HexEncode(base::make_span(data).first(3)), "FD2C01");
    EXPECT_TRUE(base::ranges::all_of(base::make_span(data).last(300),
                                     [](auto c) { return c == 0xcd; }));
  }

  {
    std::vector<uint8_t> bytes(0x10000, 0xef);
    std::vector<uint8_t> data;
    BitcoinSerializerStream stream(data);
    stream.PushSizeAndBytes(bytes);
    EXPECT_EQ(data.size(), 5u + 0x10000);
    EXPECT_EQ(base::HexEncode(base::make_span(data).first(5)), "FE00000100");
    EXPECT_TRUE(base::ranges::all_of(base::make_span(data).last(0x10000),
                                     [](auto c) { return c == 0xef; }));
  }
}

TEST(BitcoinSerializerStream, PushBytes) {
  std::vector<uint8_t> bytes({0x01, 0x02, 0xab, 0xcd, 0xef});
  std::vector<uint8_t> data;
  BitcoinSerializerStream stream(data);
  stream.PushBytes(bytes);
  EXPECT_EQ(base::HexEncode(data), "0102ABCDEF");
}

TEST(BitcoinSerializerStream, PushBytesReversed) {
  std::vector<uint8_t> bytes({0x01, 0x02, 0xab, 0xcd, 0xef});
  std::vector<uint8_t> data;
  BitcoinSerializerStream stream(data);
  stream.PushBytesReversed(bytes);
  EXPECT_EQ(base::HexEncode(data), "EFCDAB0201");
}

TEST(BitcoinSerializer, SerializeInputForSign) {
  BitcoinTransaction tx;

  auto& input1 = tx.inputs().emplace_back();
  input1.utxo_address = kAddress1;
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.utxo_value = 555666777;
  input1.script_sig = {1, 2, 3};
  input1.witness = {4, 5, 6};

  auto& input2 = tx.inputs().emplace_back();
  input2.utxo_address = kAddress2;
  input2.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input2.utxo_outpoint.txid);
  input2.utxo_value = 555;
  input2.script_sig = {1, 2};
  input2.witness = {4, 5};

  auto& output1 = tx.outputs().emplace_back();
  output1.address = kAddress1;
  output1.amount = 5;

  auto& output2 = tx.outputs().emplace_back();
  output2.address = kAddress2;
  output2.amount = 50;

  tx.set_locktime(777);

  EXPECT_EQ(base::HexEncode(*BitcoinSerializer::SerializeInputForSign(tx, 0)),
            "25395E842E3005AC64B1B23CEA639C1899A3C2D18EBF58CC47679EBE3EC810F9");
  EXPECT_EQ(base::HexEncode(*BitcoinSerializer::SerializeInputForSign(tx, 1)),
            "FBD8650BA68214C9659928A7E16A6B4148D895755BC5036B328532CAFC4267FB");

  // P2PKH addresses are not suppported.
  tx.inputs()[0].utxo_address = "1N4Qbzg6LSXUXyXu2MDuGfzxwMA7do8AyL";
  EXPECT_FALSE(BitcoinSerializer::SerializeInputForSign(tx, 0));

  // P2SH addresses are not suppported.
  tx.inputs()[0].utxo_address = "3J98t1WpEZ73CNmQviecrnyiWrnqRhWNLy";
  EXPECT_FALSE(BitcoinSerializer::SerializeInputForSign(tx, 0));

  // P2TR addresses are not suppported.
  tx.inputs()[0].utxo_address =
      "bc1peu5hzzyj8cnqm05le6ag7uwry0ysmtf3v4uuxv3v8hqhvsatca8ss2vuwx";
  EXPECT_FALSE(BitcoinSerializer::SerializeInputForSign(tx, 0));
}

TEST(BitcoinSerializer, SerializeWitness) {
  std::vector<uint8_t> signature = {0, 1, 2, 3};
  std::vector<uint8_t> pubkey = {0xaa, 0xbb, 0xcc, 0xdd};
  EXPECT_EQ(
      base::HexEncode(BitcoinSerializer::SerializeWitness(signature, pubkey)),
      "02040001020304AABBCCDD");
}

TEST(BitcoinSerializer, SerializeSignedTransaction) {
  std::vector<uint8_t> signature = {0, 1, 2, 3};
  std::vector<uint8_t> pubkey = {0xaa, 0xbb, 0xcc, 0xdd};

  BitcoinTransaction tx;

  auto& input1 = tx.inputs().emplace_back();
  input1.utxo_address = kAddress1;
  input1.utxo_outpoint.index = 123;
  base::HexStringToSpan(kTxid1, input1.utxo_outpoint.txid);
  input1.utxo_value = 555666777;
  input1.witness = BitcoinSerializer::SerializeWitness(signature, pubkey);

  auto& input2 = tx.inputs().emplace_back();
  input2.utxo_address = kAddress2;
  input2.utxo_outpoint.index = 7;
  base::HexStringToSpan(kTxid2, input2.utxo_outpoint.txid);
  input2.utxo_value = 555;
  input2.witness = BitcoinSerializer::SerializeWitness(signature, pubkey);

  auto& output1 = tx.outputs().emplace_back();
  output1.address = kAddress1;
  output1.amount = 5;

  auto& output2 = tx.outputs().emplace_back();
  output2.address = kAddress2;
  output2.amount = 50;

  tx.set_locktime(777);

  EXPECT_EQ(
      base::HexEncode(BitcoinSerializer::SerializeSignedTransaction(tx)),
      "02000000000102C5E29F841382F02A49BEAFAC756D14A211EC9089AD50E153767625B750"
      "8F38AA7B00000000FDFFFFFFBEC2C52B2448A8733648E967D2B4559D0F1AA4BBBB93E53E"
      "9F516A12FB9C1CBD0700000000FDFFFFFF02050000000000000016001427623E8F36A16A"
      "7D3A35A32B45B3531F6A9228C63200000000000000160014674F8F912B5A9305F5D3A348"
      "F9B069D9101173E902040001020304AABBCCDD02040001020304AABBCCDD09030000");
}

}  // namespace brave_wallet
