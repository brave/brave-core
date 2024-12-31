/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/rlp_encode.h"

#include <string>
#include <utility>

#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

base::Value RLPTestStringToValue(std::string s) {
  base::ReplaceChars(s, "'", "\"", &s);
  return base::test::ParseJson(s);
}

}  // namespace

namespace brave_wallet {

TEST(RLPEncodeTest, EmptyString) {
  auto v = RLPEncode(RLPTestStringToValue("''"));
  ASSERT_EQ(ToHex(v), "0x80");
}

TEST(RLPEncodeTest, SingleChar) {
  auto v = RLPEncode(RLPTestStringToValue("'d'"));
  ASSERT_EQ(ToHex(v), "0x64");
}

TEST(RLPEncodeTest, ShortString) {
  auto v = RLPEncode(RLPTestStringToValue("'dog'"));
  ASSERT_EQ(ToHex(v), "0x83646f67");
}

TEST(RLPEncodeTest, ShortString2) {
  auto v = RLPEncode(RLPTestStringToValue(
      "'Lorem ipsum dolor sit amet, consectetur adipisicing eli'"));
  ASSERT_EQ(ToHex(v),
            "0xb74c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e"
            "7365637465747572206164697069736963696e6720656c69");
}

TEST(RLPEncodeTest, LongString) {
  auto v = RLPEncode(RLPTestStringToValue(
      "'Lorem ipsum dolor sit amet, consectetur adipisicing elit'"));
  ASSERT_EQ(ToHex(v),
            "0xb8384c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f"
            "6e7365637465747572206164697069736963696e6720656c6974");
}

TEST(RLPEncodeTest, LongString2) {
  auto v = RLPEncode(RLPTestStringToValue(
      "'Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur "
      "mauris magna, suscipit sed vehicula non, iaculis faucibus tortor. Proin "
      "suscipit ultricies malesuada. Duis tortor elit, dictum quis tristique "
      "eu, ultrices at risus. Morbi a est imperdiet mi ullamcorper aliquet "
      "suscipit nec lorem. Aenean quis leo mollis, vulputate elit varius, "
      "consequat enim. Nulla ultrices turpis justo, et posuere urna "
      "consectetur nec. Proin non convallis metus. Donec tempor ipsum in "
      "mauris congue sollicitudin. Vestibulum ante ipsum primis in faucibus "
      "orci luctus et ultrices posuere cubilia Curae; Suspendisse convallis "
      "sem vel massa faucibus, eget lacinia lacus tempor. Nulla quis ultricies "
      "purus. Proin auctor rhoncus nibh condimentum mollis. Aliquam consequat "
      "enim at metus luctus, a eleifend purus egestas. Curabitur at nibh "
      "metus. Nam bibendum, neque at auctor tristique, lorem libero aliquet "
      "arcu, non interdum tellus lectus sit amet eros. Cras rhoncus, metus ac "
      "ornare cursus, dolor justo ultrices metus, at ullamcorper volutpat'"));
  ASSERT_EQ(
      ToHex(v),
      "0xb904004c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e73"
      "656374657475722061646970697363696e6720656c69742e20437572616269747572206d"
      "6175726973206d61676e612c20737573636970697420736564207665686963756c61206e"
      "6f6e2c20696163756c697320666175636962757320746f72746f722e2050726f696e2073"
      "7573636970697420756c74726963696573206d616c6573756164612e204475697320746f"
      "72746f7220656c69742c2064696374756d2071756973207472697374697175652065752c"
      "20756c7472696365732061742072697375732e204d6f72626920612065737420696d7065"
      "7264696574206d6920756c6c616d636f7270657220616c69717565742073757363697069"
      "74206e6563206c6f72656d2e2041656e65616e2071756973206c656f206d6f6c6c69732c"
      "2076756c70757461746520656c6974207661726975732c20636f6e73657175617420656e"
      "696d2e204e756c6c6120756c74726963657320747572706973206a7573746f2c20657420"
      "706f73756572652075726e6120636f6e7365637465747572206e65632e2050726f696e20"
      "6e6f6e20636f6e76616c6c6973206d657475732e20446f6e65632074656d706f72206970"
      "73756d20696e206d617572697320636f6e67756520736f6c6c696369747564696e2e2056"
      "6573746962756c756d20616e746520697073756d207072696d697320696e206661756369"
      "627573206f726369206c756374757320657420756c74726963657320706f737565726520"
      "637562696c69612043757261653b2053757370656e646973736520636f6e76616c6c6973"
      "2073656d2076656c206d617373612066617563696275732c2065676574206c6163696e69"
      "61206c616375732074656d706f722e204e756c6c61207175697320756c74726963696573"
      "2070757275732e2050726f696e20617563746f722072686f6e637573206e69626820636f"
      "6e64696d656e74756d206d6f6c6c69732e20416c697175616d20636f6e73657175617420"
      "656e696d206174206d65747573206c75637475732c206120656c656966656e6420707572"
      "757320656765737461732e20437572616269747572206174206e696268206d657475732e"
      "204e616d20626962656e64756d2c206e6571756520617420617563746f72207472697374"
      "697175652c206c6f72656d206c696265726f20616c697175657420617263752c206e6f6e"
      "20696e74657264756d2074656c6c7573206c65637475732073697420616d65742065726f"
      "732e20437261732072686f6e6375732c206d65747573206163206f726e61726520637572"
      "7375732c20646f6c6f72206a7573746f20756c747269636573206d657475732c20617420"
      "756c6c616d636f7270657220766f6c7574706174");
}

TEST(RLPEncodeTest, Zero) {
  auto v = RLPEncode(RLPTestStringToValue("0"));
  ASSERT_EQ(ToHex(v), "0x80");
}

TEST(RLPEncodeTest, SmallInt1) {
  auto v = RLPEncode(RLPTestStringToValue("1"));
  ASSERT_EQ(ToHex(v), "0x01");
}

TEST(RLPEncodeTest, SmallInt2) {
  auto v = RLPEncode(RLPTestStringToValue("16"));
  ASSERT_EQ(ToHex(v), "0x10");
}

TEST(RLPEncodeTest, SmallInt3) {
  auto v = RLPEncode(RLPTestStringToValue("79"));
  ASSERT_EQ(ToHex(v), "0x4f");
}

TEST(RLPEncodeTest, SmallInt4) {
  auto v = RLPEncode(RLPTestStringToValue("127"));
  ASSERT_EQ(ToHex(v), "0x7f");
}

TEST(RLPEncodeTest, MediumInt1) {
  auto v = RLPEncode(RLPTestStringToValue("128"));
  ASSERT_EQ(ToHex(v), "0x8180");
}

TEST(RLPEncodeTest, MediumInt2) {
  auto v = RLPEncode(RLPTestStringToValue("1000"));
  ASSERT_EQ(ToHex(v), "0x8203e8");
}

TEST(RLPEncodeTest, MediumInt3) {
  auto v = RLPEncode(RLPTestStringToValue("100000"));
  ASSERT_EQ(ToHex(v), "0x830186a0");
}

TEST(RLPEncodeTest, BlobStorage) {
  base::Value::BlobStorage input{0, 255, 33, 127, 0, 128};
  auto v = RLPEncode(base::Value(input));
  ASSERT_EQ(ToHex(v), "0x8600ff217f0080");
}

TEST(RLPEncodeTest, MediumInt4) {
  // 83729609699884896815286331701780722
  uint256_t ten_billion = static_cast<uint256_t>(10000000000);
  uint256_t input = static_cast<uint256_t>(83729);
  input *= ten_billion;
  input += static_cast<uint256_t>(6096998848);
  input *= ten_billion;
  input += static_cast<uint256_t>(9681528633);
  input *= ten_billion;
  input += static_cast<uint256_t>(1701780722);
  auto v = RLPEncode(base::Value(RLPUint256ToBlob(input)));
  ASSERT_EQ(ToHex(v), "0x8f102030405060708090a0b0c0d0e0f2");
}

TEST(RLPEncodeTest, MediumInt5) {
  // 105315505618206987246253880190783558935785933862974822347068935681
  uint256_t ten_billion = static_cast<uint256_t>(10000000000);
  uint256_t input = static_cast<uint256_t>(105315);
  input *= ten_billion;
  input += static_cast<uint256_t>(5056182069);
  input *= ten_billion;
  input += static_cast<uint256_t>(8724625388);
  input *= ten_billion;
  input += static_cast<uint256_t>(190783558);
  input *= ten_billion;
  input += static_cast<uint256_t>(9357859338);
  input *= ten_billion;
  input += static_cast<uint256_t>(6297482234);
  input *= ten_billion;
  input += static_cast<uint256_t>(7068935681);
  auto v = RLPEncode(base::Value(RLPUint256ToBlob(input)));
  ASSERT_EQ(ToHex(v),
            "0x9c0100020003000400050006000700080009000a000b000c000d000e01");
}

TEST(RLPEncodeTest, BigInt) {
  // 115792089237316195423570985008687907853269984665640564039457584007913129639935
  uint256_t ten_billion = static_cast<uint256_t>(10000000000);
  uint256_t input = static_cast<uint256_t>(11579208);
  input *= ten_billion;
  input += static_cast<uint256_t>(9237316195);
  input *= ten_billion;
  input += static_cast<uint256_t>(4235709850);
  input *= ten_billion;
  input += static_cast<uint256_t>(868790785);
  input *= ten_billion;
  input += static_cast<uint256_t>(3269984665);
  input *= ten_billion;
  input += static_cast<uint256_t>(6405640394);
  input *= ten_billion;
  input += static_cast<uint256_t>(5758400791);
  input *= ten_billion;
  input += static_cast<uint256_t>(3129639935);
  auto v = RLPEncode(base::Value(RLPUint256ToBlob(input)));
  ASSERT_EQ(
      ToHex(v),
      "0xa0ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
}

TEST(RLPEncodeTest, ByteString00) {
  std::string bytestring = {0};
  auto v = RLPEncode(base::Value(bytestring));
  ASSERT_EQ(ToHex(v), "0x00");
}

TEST(RLPEncodeTest, ByteString01) {
  std::string bytestring = {1};
  auto v = RLPEncode(base::Value(bytestring));
  ASSERT_EQ(ToHex(v), "0x01");
}

TEST(RLPEncodeTest, ByteString7f) {
  std::string bytestring = {0x7f};
  auto v = RLPEncode(base::Value(bytestring));
  ASSERT_EQ(ToHex(v), "0x7f");
}

TEST(RLPEncodeTest, EmptyList) {
  auto v = RLPEncode(RLPTestStringToValue("[]"));
  ASSERT_EQ(ToHex(v), "0xc0");
}

TEST(RLPEncodeTest, StringList) {
  auto v = RLPEncode(RLPTestStringToValue("['dog', 'god', 'cat']"));
  ASSERT_EQ(ToHex(v), "0xcc83646f6783676f6483636174");
}

TEST(RLPEncodeTest, ShortListMax1) {
  auto v = RLPEncode(
      RLPTestStringToValue("['asdf', 'qwer', 'zxcv', 'asdf', 'qwer', 'zxcv', "
                           "'asdf', 'qwer', 'zxcv', 'asdf', 'qwer']"));
  ASSERT_EQ(ToHex(v),
            "0xf784617364668471776572847a78637684617364668471776572847a78637684"
            "617364668471776572847a78637684617364668471776572");
}

TEST(RLPEncodeTest, LongList1) {
  auto v = RLPEncode(RLPTestStringToValue(
      "[['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', "
      "'zxcv'], ['asdf', 'qwer', 'zxcv']]"));
  ASSERT_EQ(
      ToHex(v),
      "0xf840cf84617364668471776572847a786376cf84617364668471776572847a786376cf"
      "84617364668471776572847a786376cf84617364668471776572847a786376");
}

TEST(RLPEncodeTest, LongList2) {
  auto v = RLPEncode(RLPTestStringToValue(
      "[['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', "
      "'zxcv'], ['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', 'zxcv'], ['asdf', "
      "'qwer', 'zxcv'], ['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', 'zxcv'], "
      "['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', "
      "'zxcv'], ['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', 'zxcv'], ['asdf', "
      "'qwer', 'zxcv'], ['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', 'zxcv'], "
      "['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', "
      "'zxcv'], ['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', 'zxcv'], ['asdf', "
      "'qwer', 'zxcv'], ['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', 'zxcv'], "
      "['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', "
      "'zxcv'], ['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', 'zxcv'], ['asdf', "
      "'qwer', 'zxcv'], ['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', 'zxcv']]"));
  ASSERT_EQ(
      ToHex(v),
      "0xf90200cf84617364668471776572847a786376cf84617364668471776572847a786376"
      "cf84617364668471776572847a786376cf84617364668471776572847a786376cf846173"
      "64668471776572847a786376cf84617364668471776572847a786376cf84617364668471"
      "776572847a786376cf84617364668471776572847a786376cf8461736466847177657284"
      "7a786376cf84617364668471776572847a786376cf84617364668471776572847a786376"
      "cf84617364668471776572847a786376cf84617364668471776572847a786376cf846173"
      "64668471776572847a786376cf84617364668471776572847a786376cf84617364668471"
      "776572847a786376cf84617364668471776572847a786376cf8461736466847177657284"
      "7a786376cf84617364668471776572847a786376cf84617364668471776572847a786376"
      "cf84617364668471776572847a786376cf84617364668471776572847a786376cf846173"
      "64668471776572847a786376cf84617364668471776572847a786376cf84617364668471"
      "776572847a786376cf84617364668471776572847a786376cf8461736466847177657284"
      "7a786376cf84617364668471776572847a786376cf84617364668471776572847a786376"
      "cf84617364668471776572847a786376cf84617364668471776572847a786376cf846173"
      "64668471776572847a786376");
}

TEST(RLPEncodeTest, Multilist) {
  auto v = RLPEncode(RLPTestStringToValue("['zw', [4], 1]"));
  ASSERT_EQ(ToHex(v), "0xc6827a77c10401");
}

TEST(RLPEncodeTest, ListOfLists) {
  auto v = RLPEncode(RLPTestStringToValue("[[[], []], []]"));
  ASSERT_EQ(ToHex(v), "0xc4c2c0c0c0");
}

TEST(RLPEncodeTest, ListOfLists2) {
  auto v = RLPEncode(RLPTestStringToValue("[[], [[]], [[], [[]]]]"));
  ASSERT_EQ(ToHex(v), "0xc7c0c1c0c3c0c1c0");
}

TEST(RLPEncodeTest, DictTest1) {
  auto v = RLPEncode(
      RLPTestStringToValue("[['key1', 'val1'], ['key2', 'val2'], ['key3', "
                           "'val3'], ['key4', 'val4']]"));
  ASSERT_EQ(ToHex(v),
            "0xecca846b6579318476616c31ca846b6579328476616c32ca846b657933847661"
            "6c33ca846b6579348476616c34");
}

TEST(RLPEncodeTest, ComplexStructure) {
  auto v = RLPEncode(RLPTestStringToValue(
      "['cat',['puppy', 'cow'], 'horse', [[]], 'pig', [''], 'sheep']"));
  ASSERT_EQ(ToHex(v),
            "0xe383636174ca85707570707983636f7785686f727365c1c083706967c1808573"
            "68656570");
}

TEST(RLPEncodeTest, DictionaryValueNotSupported) {
  base::Value::Dict d;
  d.Set("test", true);
  ASSERT_TRUE(brave_wallet::RLPEncode(base::Value(std::move(d))).empty());
}

}  // namespace brave_wallet
