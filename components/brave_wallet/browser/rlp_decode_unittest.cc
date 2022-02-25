/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/rlp_decode.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

std::string RLPTestValueToString(const base::Value& val) {
  std::string output;
  if (val.is_string()) {
    output = "'";
    std::string s = val.GetString();
    output += s + "'";
  } else if (val.is_list()) {
    if (output.size()) {
      output += ", ";
    }
    output += "[";
    std::string elems;
    const base::ListValue* list;
    val.GetAsList(&list);
    for (size_t i = 0; i < list->GetListDeprecated().size(); i++) {
      const auto& child = list->GetListDeprecated();
      if (child.size() <= i) {
        continue;
      }
      if (elems.size()) {
        elems += ", ";
      }
      elems += RLPTestValueToString(child[i]);
    }
    output += elems + "]";
  }
  return output;
}

std::string FromHex(const std::string& hex_input) {
  std::string bytes;
  base::HexStringToString(hex_input.substr(2, std::string::npos), &bytes);
  return bytes;
}

}  // namespace

namespace brave_wallet {

TEST(RLPDecodeTest, ByteString00) {
  base::Value val;
  ASSERT_TRUE(RLPDecode(FromHex("0x00"), &val));
  ASSERT_TRUE(val.is_string());
  std::string bytestring = {0};
  std::string s = val.GetString();
  ASSERT_EQ(bytestring, s);
}

TEST(RLPDecodeTest, ByteString01) {
  base::Value val;
  ASSERT_TRUE(RLPDecode(FromHex("0x01"), &val));
  ASSERT_TRUE(val.is_string());
  std::string bytestring = {1};
  std::string s = val.GetString();
  ASSERT_EQ(bytestring, s);
}

TEST(RLPDecodeTest, ByteString7f) {
  base::Value val;
  ASSERT_TRUE(RLPDecode(FromHex("0x7f"), &val));
  ASSERT_TRUE(val.is_string());
  std::string bytestring = {0x7f};
  std::string s = val.GetString();
  ASSERT_EQ(bytestring, s);
}

TEST(RLPDecodeTest, EmptyString) {
  base::Value val;
  ASSERT_TRUE(RLPDecode(FromHex("0x80"), &val));
  ASSERT_EQ("''", RLPTestValueToString(val));
}

TEST(RLPDecodeTest, SingleChar) {
  base::Value val;
  ASSERT_TRUE(RLPDecode(FromHex("0x64"), &val));
  ASSERT_EQ("'d'", RLPTestValueToString(val));
}

TEST(RLPDecodeTest, ShortString) {
  base::Value val;
  ASSERT_TRUE(RLPDecode(FromHex("0x83646f67"), &val));
  ASSERT_EQ("'dog'", RLPTestValueToString(val));
}

TEST(RLPDecodeTest, ShortString2) {
  base::Value val;
  ASSERT_TRUE(RLPDecode(
      FromHex("0xb74c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f"
              "6e7365637465747572206164697069736963696e6720656c69"),
      &val));
  ASSERT_EQ("'Lorem ipsum dolor sit amet, consectetur adipisicing eli'",
            RLPTestValueToString(val));
}

TEST(RLPDecodeTest, LongString) {
  base::Value val;
  ASSERT_TRUE(RLPDecode(
      FromHex("0xb8384c6f72656d20697073756d20646f6c6f722073697420616d65742c2063"
              "6f6e7365637465747572206164697069736963696e6720656c6974"),
      &val));
  ASSERT_EQ("'Lorem ipsum dolor sit amet, consectetur adipisicing elit'",
            RLPTestValueToString(val));
}

TEST(RLPDecodeTest, LongString2) {
  base::Value val;
  ASSERT_TRUE(RLPDecode(
      FromHex(
          "0xb904004c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f"
          "6e73656374657475722061646970697363696e6720656c69742e2043757261626974"
          "7572206d6175726973206d61676e612c207375736369706974207365642076656869"
          "63756c61206e6f6e2c20696163756c697320666175636962757320746f72746f722e"
          "2050726f696e20737573636970697420756c74726963696573206d616c6573756164"
          "612e204475697320746f72746f7220656c69742c2064696374756d20717569732074"
          "72697374697175652065752c20756c7472696365732061742072697375732e204d6f"
          "72626920612065737420696d70657264696574206d6920756c6c616d636f72706572"
          "20616c6971756574207375736369706974206e6563206c6f72656d2e2041656e6561"
          "6e2071756973206c656f206d6f6c6c69732c2076756c70757461746520656c697420"
          "7661726975732c20636f6e73657175617420656e696d2e204e756c6c6120756c7472"
          "6963657320747572706973206a7573746f2c20657420706f73756572652075726e61"
          "20636f6e7365637465747572206e65632e2050726f696e206e6f6e20636f6e76616c"
          "6c6973206d657475732e20446f6e65632074656d706f7220697073756d20696e206d"
          "617572697320636f6e67756520736f6c6c696369747564696e2e2056657374696275"
          "6c756d20616e746520697073756d207072696d697320696e20666175636962757320"
          "6f726369206c756374757320657420756c74726963657320706f7375657265206375"
          "62696c69612043757261653b2053757370656e646973736520636f6e76616c6c6973"
          "2073656d2076656c206d617373612066617563696275732c2065676574206c616369"
          "6e6961206c616375732074656d706f722e204e756c6c61207175697320756c747269"
          "636965732070757275732e2050726f696e20617563746f722072686f6e637573206e"
          "69626820636f6e64696d656e74756d206d6f6c6c69732e20416c697175616d20636f"
          "6e73657175617420656e696d206174206d65747573206c75637475732c206120656c"
          "656966656e6420707572757320656765737461732e20437572616269747572206174"
          "206e696268206d657475732e204e616d20626962656e64756d2c206e657175652061"
          "7420617563746f72207472697374697175652c206c6f72656d206c696265726f2061"
          "6c697175657420617263752c206e6f6e20696e74657264756d2074656c6c7573206c"
          "65637475732073697420616d65742065726f732e20437261732072686f6e6375732c"
          "206d65747573206163206f726e617265206375727375732c20646f6c6f72206a7573"
          "746f20756c747269636573206d657475732c20617420756c6c616d636f7270657220"
          "766f6c7574706174"),
      &val));
  ASSERT_EQ(
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
      "ornare cursus, dolor justo ultrices metus, at ullamcorper volutpat'",
      RLPTestValueToString(val));
}

TEST(RLPDecodeTest, EmptyList) {
  base::Value val;
  ASSERT_TRUE(RLPDecode(FromHex("0xc0"), &val));
  ASSERT_EQ("[]", RLPTestValueToString(val));
}

TEST(RLPDecodeTest, StringList) {
  base::Value val;
  ASSERT_TRUE(RLPDecode(FromHex("0xcc83646f6783676f6483636174"), &val));
  ASSERT_EQ("['dog', 'god', 'cat']", RLPTestValueToString(val));
}

TEST(RLPDecodeTest, ShortListMax1) {
  base::Value val;
  ASSERT_TRUE(RLPDecode(
      FromHex("0xf784617364668471776572847a78637684617364668471776572847a786376"
              "84617364668471776572847a78637684617364668471776572"),
      &val));
  ASSERT_EQ(
      "['asdf', 'qwer', 'zxcv', 'asdf', 'qwer', 'zxcv', 'asdf', 'qwer', "
      "'zxcv', 'asdf', 'qwer']",
      RLPTestValueToString(val));
}

TEST(RLPDecodeTest, LongList1) {
  base::Value val;
  ASSERT_TRUE(RLPDecode(
      FromHex(
          "0xf840cf84617364668471776572847a786376cf84617364668471776572847a7863"
          "76cf84617364668471776572847a786376cf84617364668471776572847a786376"),
      &val));
  ASSERT_EQ(
      "[['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', "
      "'zxcv'], ['asdf', 'qwer', 'zxcv']]",
      RLPTestValueToString(val));
}

TEST(RLPDecodeTest, LongList2) {
  base::Value val;
  ASSERT_TRUE(RLPDecode(
      FromHex(
          "0xf90200cf84617364668471776572847a786376cf84617364668471776572847a78"
          "6376cf84617364668471776572847a786376cf84617364668471776572847a786376"
          "cf84617364668471776572847a786376cf84617364668471776572847a786376cf84"
          "617364668471776572847a786376cf84617364668471776572847a786376cf846173"
          "64668471776572847a786376cf84617364668471776572847a786376cf8461736466"
          "8471776572847a786376cf84617364668471776572847a786376cf84617364668471"
          "776572847a786376cf84617364668471776572847a786376cf846173646684717765"
          "72847a786376cf84617364668471776572847a786376cf8461736466847177657284"
          "7a786376cf84617364668471776572847a786376cf84617364668471776572847a78"
          "6376cf84617364668471776572847a786376cf84617364668471776572847a786376"
          "cf84617364668471776572847a786376cf84617364668471776572847a786376cf84"
          "617364668471776572847a786376cf84617364668471776572847a786376cf846173"
          "64668471776572847a786376cf84617364668471776572847a786376cf8461736466"
          "8471776572847a786376cf84617364668471776572847a786376cf84617364668471"
          "776572847a786376cf84617364668471776572847a786376cf846173646684717765"
          "72847a786376"),
      &val));
  ASSERT_EQ(
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
      "'qwer', 'zxcv'], ['asdf', 'qwer', 'zxcv'], ['asdf', 'qwer', 'zxcv']]",
      RLPTestValueToString(val));
}

TEST(RLPDecodeTest, ListOfLists) {
  base::Value val;
  ASSERT_TRUE(RLPDecode(FromHex("0xc4c2c0c0c0"), &val));
  ASSERT_EQ("[[[], []], []]", RLPTestValueToString(val));
}

TEST(RLPDecodeTest, ListOfLists2) {
  base::Value val;
  ASSERT_TRUE(RLPDecode(FromHex("0xc7c0c1c0c3c0c1c0"), &val));
  ASSERT_EQ("[[], [[]], [[], [[]]]]", RLPTestValueToString(val));
}

TEST(RLPDecodeTest, DictTest1) {
  base::Value val;
  ASSERT_TRUE(
      RLPDecode(FromHex("0xecca846b6579318476616c31ca846b6579328476616c32ca846b"
                        "6579338476616c33ca846b6579348476616c34"),
                &val));
  ASSERT_EQ(
      "[['key1', 'val1'], ['key2', 'val2'], ['key3', 'val3'], ['key4', "
      "'val4']]",
      RLPTestValueToString(val));
}

TEST(RLPDecodeTest, ComplexStructure) {
  base::Value val;
  ASSERT_TRUE(RLPDecode(FromHex("0xe383636174ca85707570707983636f7785686f727365"
                                "c1c083706967c180857368656570"),
                        &val));
  ASSERT_EQ("['cat', ['puppy', 'cow'], 'horse', [[]], 'pig', [''], 'sheep']",
            RLPTestValueToString(val));
}

TEST(RLPDecodeTest, InvalidInputInt32Overflow) {
  base::Value val;
  ASSERT_FALSE(RLPDecode(FromHex("0xbf0f000000000000021111"), &val));
  ASSERT_TRUE(val.is_none());
}

TEST(RLPDecodeTest, InvalidInputInt32Overflow2) {
  base::Value val;
  ASSERT_FALSE(RLPDecode(FromHex("0xff0f000000000000021111"), &val));
  ASSERT_TRUE(val.is_none());
}

TEST(RLPDecodeTest, InvalidInputWrongSizeList) {
  base::Value val;
  ASSERT_FALSE(RLPDecode(FromHex("0xf80180"), &val));
  ASSERT_TRUE(val.is_none());
}

TEST(RLPDecodeTest, InvalidInputwrongSizeList2) {
  base::Value val;
  ASSERT_FALSE(RLPDecode(FromHex("0xf80100"), &val));
  ASSERT_TRUE(val.is_none());
}

TEST(RLPDecodeTest, InvalidInputIncorrectLengthInArray) {
  base::Value val;
  ASSERT_FALSE(RLPDecode(FromHex("0xb9002100dc2b275d0f74e8a53e6f4ec61b27f242788"
                                 "20be3f82ea2110e582081b0565df0"),
                         &val));
  ASSERT_TRUE(val.is_none());
}

TEST(RLPDecodeTest, InvalidInputRandomRLP) {
  base::Value val;
  ASSERT_FALSE(RLPDecode(
      FromHex(
          "0xf861f83eb9002100dc2b275d0f74e8a53e6f4ec61b27f24278820be3f82ea2110e"
          "582081b0565df027b90015002d5ef8325ae4d034df55d4b58d0dfba64d61ddd17be0"
          "0000b9001a00dae30907045a2f66fa36f2bb8aa9029cbb0b8a7b3b5c435ab331"),
      &val));
  ASSERT_TRUE(val.is_none());
}

TEST(RLPDecodeTest, InvalidInputBytesShouldBeSingleByte00) {
  base::Value val;
  ASSERT_FALSE(RLPDecode(FromHex("0x8100"), &val));
  ASSERT_TRUE(val.is_none());
}

TEST(RLPDecodeTest, InvalidInputBytesShouldBeSingleByte01) {
  base::Value val;
  ASSERT_FALSE(RLPDecode(FromHex("0x8101"), &val));
  ASSERT_TRUE(val.is_none());
}

TEST(RLPDecodeTest, InvalidInputBytesShouldBeSingleByte7F) {
  base::Value val;
  ASSERT_FALSE(RLPDecode(FromHex("0x817F"), &val));
  ASSERT_TRUE(val.is_none());
}

TEST(RLPDecodeTest, InvalidInputEmptyEncoding) {
  base::Value val;
  ASSERT_FALSE(RLPDecode("", &val));
  ASSERT_TRUE(val.is_none());
}

TEST(RLPDecodeTest, InvalidInputLessThanShortLengthArray1) {
  base::Value val;
  ASSERT_FALSE(RLPDecode(FromHex("0x81"), &val));
  ASSERT_TRUE(val.is_none());
}

TEST(RLPDecodeTest, InvalidInputLessThanShortLengthArray2) {
  base::Value val;
  ASSERT_FALSE(RLPDecode(
      FromHex(
          "0xa0000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e"),
      &val));
  ASSERT_TRUE(val.is_none());
}

TEST(RLPDecodeTest, InvalidInputLessThanShortLengthList1) {
  base::Value val;
  ASSERT_FALSE(RLPDecode(FromHex("0xc5010203"), &val));
  ASSERT_TRUE(val.is_none());
}

TEST(RLPDecodeTest, InvalidInputLessThanShortLengthList2) {
  base::Value val;
  ASSERT_FALSE(RLPDecode(FromHex("0xe201020304050607"), &val));
  ASSERT_TRUE(val.is_none());
}

TEST(RLPDecodeTest, InvalidInputLessThanLongLengthArray1) {
  base::Value val;
  ASSERT_FALSE(RLPDecode(FromHex("0xba010000aabbccddeeff"), &val));
  ASSERT_TRUE(val.is_none());
}

TEST(RLPDecodeTest, InvalidInputLessThanLongLengthArray2) {
  base::Value val;
  ASSERT_FALSE(
      RLPDecode(FromHex("0xb840ffeeddccbbaa99887766554433221100"), &val));
  ASSERT_TRUE(val.is_none());
}

TEST(RLPDecodeTest, InvalidInputLessThanLongLengthList1) {
  base::Value val;
  ASSERT_FALSE(RLPDecode(FromHex("0xf90180"), &val));
  ASSERT_TRUE(val.is_none());
}

TEST(RLPDecodeTest, InvalidInputLessThanLongLengthList2) {
  base::Value val;
  ASSERT_FALSE(
      RLPDecode(FromHex("0xffffffffffffffffff0001020304050607"), &val));
  ASSERT_TRUE(val.is_none());
}

}  // namespace brave_wallet
