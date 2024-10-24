/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/eth_sign_typed_data_helper.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/test/values_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJson;

namespace brave_wallet {

TEST(EthSignedTypedDataHelperUnitTest, EncodeTypes) {
  const std::string types_json(R"({
    "Mail": [
        {"name": "from", "type": "Person"},
        {"name": "to", "type": "Person"},
        {"name": "contents", "type": "string"},
    ],
    "Person": [
        {"name": "name", "type": "string"},
        {"name": "wallet", "type": "address"}
    ]})");

  auto types_value = ParseJson(types_json);

  std::unique_ptr<EthSignTypedDataHelper> helper =
      EthSignTypedDataHelper::Create(types_value.GetDict().Clone(),
                                     EthSignTypedDataHelper::Version::kV4);
  ASSERT_TRUE(helper);
  const std::string encoded_types_v4 = helper->EncodeTypes("Mail");
  EXPECT_EQ(encoded_types_v4,
            "Mail(Person from,Person to,string contents)Person(string "
            "name,address wallet)");
  auto typed_hash_v4 = helper->GetTypeHash("Mail");
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(typed_hash_v4)),
            "a0cedeb2dc280ba39b857546d74f5549c3a1d7bdc2dd96bf881f76108e23dac2");

  // v3 should be same as v4
  helper->SetVersion(EthSignTypedDataHelper::Version::kV3);
  const std::string encoded_types_v3 = helper->EncodeTypes("Mail");
  EXPECT_EQ(encoded_types_v4, encoded_types_v3);
  auto typed_hash_v3 = helper->GetTypeHash("Mail");
  EXPECT_EQ(typed_hash_v3, typed_hash_v4);

  // When depended type is not valid
  types_value = ParseJson(R"({
    "Mail": [
        {"name": "from", "type": "Person"},
        {"name": "to", "type": "Person"},
        {"name": "contents", "type": "string"},
    ],
    "Person": [
        {"name": "name", "type": "string"},
        [ "name", "type" ]
    ]})");
  helper = EthSignTypedDataHelper::Create(types_value.GetDict().Clone(),
                                          EthSignTypedDataHelper::Version::kV4);
  ASSERT_TRUE(helper);
  EXPECT_EQ(helper->EncodeTypes("Mail"),
            "Mail(Person from,Person to,string contents)");
}

TEST(EthSignedTypedDataHelperUnitTest, InvalidEncodeTypes) {
  for (const std::string& invalid_json : {
           R"({
    "Domain": [
        { "name": ["AStringArray", "String2"], "type": "string" }
    ]})",
           R"({
    "Domain": [
        { "name": 1234, "type": "uint2556" }
    ]})",
           R"({
    "Domain": [
        { "name": { "name": "name" }, "type": "string" }
    ]})",
           R"({
    "Domain": [
        { "name": "name", "type": 1234 }
    ]})",
           R"({
    "Domain": [
        {"name": "name", "type": "string"},
        [ "name", "type" ]
    ]})"}) {
    SCOPED_TRACE(invalid_json);
    auto invalid_value = ParseJson(invalid_json);
    std::unique_ptr<EthSignTypedDataHelper> invalid_types_helper =
        EthSignTypedDataHelper::Create(invalid_value.GetDict().Clone(),
                                       EthSignTypedDataHelper::Version::kV4);
    const std::string invalid_encoded_types_v4 =
        invalid_types_helper->EncodeTypes("Domain");
    EXPECT_EQ(invalid_encoded_types_v4, "");
  }
}

TEST(EthSignedTypedDataHelperUnitTest, EncodeTypesArrays) {
  const std::string types_json(R"({
    "Mail": [
        {"name": "to", "type": "Person[]"},
    ],
    "Person": [
        {"name": "name", "type": "string"},
        {"name": "wallet", "type": "address"}
    ]})");

  auto types_value = ParseJson(types_json);
  ASSERT_TRUE(types_value.is_dict());

  std::unique_ptr<EthSignTypedDataHelper> helper =
      EthSignTypedDataHelper::Create(types_value.GetDict().Clone(),
                                     EthSignTypedDataHelper::Version::kV4);
  ASSERT_TRUE(helper);
  const std::string encoded_types_v4 = helper->EncodeTypes("Mail");
  EXPECT_EQ(encoded_types_v4,
            "Mail(Person[] to)Person(string name,address wallet)");
  auto typed_hash_v4 = helper->GetTypeHash("Mail");
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(typed_hash_v4)),
            "08dde06d30a2d7c005e313f9d36bef353674e06b4ae1a923fb086f2da5b40cce");

  // v3 should be same as v4
  helper->SetVersion(EthSignTypedDataHelper::Version::kV3);
  const std::string encoded_types_v3 = helper->EncodeTypes("Mail");
  EXPECT_EQ(encoded_types_v4, encoded_types_v3);
  auto typed_hash_v3 = helper->GetTypeHash("Mail");
  EXPECT_EQ(typed_hash_v3, typed_hash_v4);
}

TEST(EthSignedTypedDataHelperUnitTest, EncodedData) {
  const std::string types_json(R"({
    "Mail": [
        {"name": "from", "type": "Person"},
        {"name": "to", "type": "Person"},
        {"name": "contents", "type": "string"}
    ],
    "Person": [
        {"name": "name", "type": "string"},
        {"name": "wallet", "type": "address"}
    ]})");
  const std::string data_json(R"({
    "from":{"name":"Cow","wallet":"0xCD2a3d9F938E13CD947Ec05AbC7FE734Df8DD826"},
    "to":{"name":"Bob","wallet":"0xbBbBBBBbbBBBbbbBbbBbbbbBBbBbbbbBbBbbBBbB"},
    "contents":"Hello, Bob!"
    })");
  auto types_value = ParseJson(types_json);
  auto data_value = ParseJson(data_json);
  auto& data_dict = data_value.GetDict();

  std::unique_ptr<EthSignTypedDataHelper> helper =
      EthSignTypedDataHelper::Create(types_value.GetDict().Clone(),
                                     EthSignTypedDataHelper::Version::kV4);
  ASSERT_TRUE(helper);
  auto encoded_mail_v4 = helper->EncodeData("Mail", data_dict);
  ASSERT_TRUE(encoded_mail_v4);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(encoded_mail_v4->first)),
            "a0cedeb2dc280ba39b857546d74f5549c3a1d7bdc2dd96bf881f76108e23dac2"
            "fc71e5fa27ff56c350aa531bc129ebdf613b772b6604664f5d8dbe21b85eb0c8cd"
            "54f074a4af31b4411ff6a60c9719dbd559c221c8ac3492d9d872b041d703d1b5aa"
            "df3154a261abdd9086fc627b61efca26ae5702701d05cd2305f7c52a2fc8");
  auto data_mail_hash_v4 = helper->HashStruct("Mail", data_dict);
  ASSERT_TRUE(data_mail_hash_v4);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(data_mail_hash_v4->first)),
            "c52c0ee5d84264471806290a3f2c4cecfc5490626bf912d01f240d7a274b371e");
  auto encoded_person_v4 =
      helper->EncodeData("Person", *(data_dict.FindDict("to")));
  ASSERT_TRUE(encoded_person_v4);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(encoded_person_v4->first)),
            "b9d8c78acf9b987311de6c7b45bb6a9c8e1bf361fa7fd3467a2163f994c79500"
            "28cac318a86c8a0a6a9156c2dba2c8c2363677ba0514ef616592d81557e679b600"
            "0000000000000000000000bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");

  // v3 should be same as v4
  helper->SetVersion(EthSignTypedDataHelper::Version::kV3);
  auto encoded_mail_v3 = helper->EncodeData("Mail", data_dict);
  ASSERT_TRUE(encoded_mail_v3);
  EXPECT_EQ(encoded_mail_v4, encoded_mail_v3);
  auto encoded_person_v3 =
      helper->EncodeData("Person", *(data_dict.FindDict("to")));
  EXPECT_EQ(encoded_person_v4->first, encoded_person_v3->first);

  // Invalid primary type name
  EXPECT_FALSE(helper->EncodeData("Brave", data_dict));

  // Extra fields in data should be ignored
  data_dict.Set("extra", base::Value("extra"));
  data_mail_hash_v4 = helper->HashStruct("Mail", data_dict);
  ASSERT_TRUE(data_mail_hash_v4);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(data_mail_hash_v4->first)),
            "c52c0ee5d84264471806290a3f2c4cecfc5490626bf912d01f240d7a274b371e");

  std::string sanitized_data_mail_v4;
  base::JSONWriter::Write(data_mail_hash_v4->second, &sanitized_data_mail_v4);
  EXPECT_EQ(
      sanitized_data_mail_v4,
      "{\"contents\":\"Hello, "
      "Bob!\",\"from\":{\"name\":\"Cow\",\"wallet\":"
      "\"0xCD2a3d9F938E13CD947Ec05AbC7FE734Df8DD826\"},\"to\":{\"name\":"
      "\"Bob\",\"wallet\":\"0xbBbBBBBbbBBBbbbBbbBbbbbBBbBbbbbBbBbbBBbB\"}}");
}

TEST(EthSignedTypedDataHelperUnitTest, InvalidEncodedData) {
  const std::string data_json(R"({"name":"Cow"})");

  auto data_value = ParseJson(data_json);
  auto& data_dict = data_value.GetDict();

  for (const std::string& invalid_json : {
           R"({
    "Domain": [
        { "name": ["AStringArray", "String2"], "type": "string" }
    ]})",
           R"({
    "Domain": [
        { "name": 1234, "type": "uint2556" }
    ]})",
           R"({
    "Domain": [
        { "name": { "name": "name" }, "type": "string" }
    ]})",
           R"({
    "Domain": [
        { "name": "name", "type": 1234 }
    ]})"}) {
    SCOPED_TRACE(invalid_json);
    auto invalid_value = ParseJson(invalid_json);

    std::unique_ptr<EthSignTypedDataHelper> invalid_types_helper =
        EthSignTypedDataHelper::Create(invalid_value.GetDict().Clone(),
                                       EthSignTypedDataHelper::Version::kV4);
    auto encoded_domain_v4 =
        invalid_types_helper->EncodeData("Domain", data_dict);
    EXPECT_FALSE(encoded_domain_v4);
  }
}

TEST(EthSignedTypedDataHelperUnitTest, RecursiveCustomTypes) {
  const std::string types_json(R"({
    "Mail": [
        {"name": "from", "type": "Person"},
        {"name": "to", "type": "Person"},
        {"name": "contents", "type": "string"},
        {"name": "replyTo", "type": "Mail"}
    ],
    "Person": [
        {"name": "name", "type": "string"},
        {"name": "wallet", "type": "address"}
    ]})");
  const std::string data_json(R"({
    "from":{"name":"Cow","wallet":"0xCD2a3d9F938E13CD947Ec05AbC7FE734Df8DD826"},
    "to":{"name":"Bob","wallet":"0xbBbBBBBbbBBBbbbBbbBbbbbBBbBbbbbBbBbbBBbB"},
    "contents":"Hello, Bob!",
    "replyTo": {
      "from": {"name": "Bob",
               "wallet": "0xbBbBBBBbbBBBbbbBbbBbbbbBBbBbbbbBbBbbBBbB"},
      "to": {"name": "Cow",
             "wallet": "0xCD2a3d9F938E13CD947Ec05AbC7FE734Df8DD826"},
      "contents": "Hello Cow"
     }
    })");
  auto types_value = ParseJson(types_json);
  auto data_value = ParseJson(data_json);
  auto& data_dict = data_value.GetDict();

  std::unique_ptr<EthSignTypedDataHelper> helper =
      EthSignTypedDataHelper::Create(types_value.GetDict().Clone(),
                                     EthSignTypedDataHelper::Version::kV4);
  ASSERT_TRUE(helper);
  auto encoded_data_v4 = helper->EncodeData("Mail", data_dict);
  ASSERT_TRUE(encoded_data_v4);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(encoded_data_v4->first)),
            "66658e9662034bcd21df657297dab8ba47f0ae05dd8aa253cc935d9aacfd9d10fc"
            "71e5fa27ff56c350aa531bc129ebdf613b772b6604664f5d8dbe21b85eb0c8cd54"
            "f074a4af31b4411ff6a60c9719dbd559c221c8ac3492d9d872b041d703d1b5aadf"
            "3154a261abdd9086fc627b61efca26ae5702701d05cd2305f7c52a2fc8ed72793e"
            "a6e1bae312dead22c15863b41b67128e0e130ca6d330d302f6d15bc1");

  // v3 and v4 handles resursive types differently
  helper->SetVersion(EthSignTypedDataHelper::Version::kV3);
  auto encoded_data_v3 = helper->EncodeData("Mail", data_dict);
  ASSERT_TRUE(encoded_data_v3);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(encoded_data_v3->first)),
            "66658e9662034bcd21df657297dab8ba47f0ae05dd8aa253cc935d9aacfd9d10fc"
            "71e5fa27ff56c350aa531bc129ebdf613b772b6604664f5d8dbe21b85eb0c8cd54"
            "f074a4af31b4411ff6a60c9719dbd559c221c8ac3492d9d872b041d703d1b5aadf"
            "3154a261abdd9086fc627b61efca26ae5702701d05cd2305f7c52a2fc8574747e4"
            "62dfdd0a5bbff373d3fcedef5483dba85f0afc5a154f4e4bb5e9ff94");
  EXPECT_NE(encoded_data_v4, encoded_data_v3);
}

TEST(EthSignedTypedDataHelperUnitTest, MissingFieldInData) {
  const std::string types_json(R"({
    "Mail": [
        {"name": "from", "type": "Person"},
        {"name": "to", "type": "Person"},
        {"name": "contents", "type": "string"}
    ],
    "Person": [
        {"name": "name", "type": "string"},
        {"name": "wallet", "type": "address"}
    ]})");
  const std::string data_json(R"({
    "to":{"name":"Bob","wallet":"0xbBbBBBBbbBBBbbbBbbBbbbbBBbBbbbbBbBbbBBbB"},
    "contents":"Hello, Bob!"
    })");

  auto types_value = ParseJson(types_json);
  auto data_value = ParseJson(data_json);
  auto& data_dict = data_value.GetDict();

  std::unique_ptr<EthSignTypedDataHelper> helper =
      EthSignTypedDataHelper::Create(types_value.GetDict().Clone(),
                                     EthSignTypedDataHelper::Version::kV4);
  ASSERT_TRUE(helper);
  auto encoded_data_v4 = helper->EncodeData("Mail", data_dict);
  ASSERT_TRUE(encoded_data_v4);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(encoded_data_v4->first)),
            "a0cedeb2dc280ba39b857546d74f5549c3a1d7bdc2dd96bf881f76108e23dac200"
            "00000000000000000000000000000000000000000000000000000000000000cd54"
            "f074a4af31b4411ff6a60c9719dbd559c221c8ac3492d9d872b041d703d1b5aadf"
            "3154a261abdd9086fc627b61efca26ae5702701d05cd2305f7c52a2fc8");

  // v3 and v4 handles resursive types differently
  helper->SetVersion(EthSignTypedDataHelper::Version::kV3);
  auto encoded_data_v3 = helper->EncodeData("Mail", data_dict);
  ASSERT_TRUE(encoded_data_v3);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(encoded_data_v3->first)),
            "a0cedeb2dc280ba39b857546d74f5549c3a1d7bdc2dd96bf881f76108e23dac2cd"
            "54f074a4af31b4411ff6a60c9719dbd559c221c8ac3492d9d872b041d703d1b5aa"
            "df3154a261abdd9086fc627b61efca26ae5702701d05cd2305f7c52a2fc8");
  EXPECT_NE(encoded_data_v4, encoded_data_v3);
}

TEST(EthSignedTypedDataHelperUnitTest, ArrayTypes) {
  const std::string types_json(R"({
    "Mail": [
        {"name": "from", "type": "Person"},
        {"name": "to", "type": "Person[]"},
        {"name": "contents", "type": "string"}
    ],
    "Person": [
        {"name": "name", "type": "string"},
        {"name": "wallet", "type": "address"}
    ]})");
  const std::string data_json(R"({
    "from":{"name":"Cow","wallet":"0xCD2a3d9F938E13CD947Ec05AbC7FE734Df8DD826"},
    "to":[
      {"name":"Bob","wallet":"0xbBbBBBBbbBBBbbbBbbBbbbbBBbBbbbbBbBbbBBbB"},
      {"name":"Alice","wallet":"0xaAaAAAAaaAAAaaaAaaAaaaaAAaAaaaaAaAaaAAaA"},
    ],
    "contents":"Hello, Alice & Bob!"
    })");

  auto types_value = ParseJson(types_json);
  auto data_value = ParseJson(data_json);
  auto& data_dict = data_value.GetDict();

  std::unique_ptr<EthSignTypedDataHelper> helper =
      EthSignTypedDataHelper::Create(types_value.GetDict().Clone(),
                                     EthSignTypedDataHelper::Version::kV4);
  ASSERT_TRUE(helper);
  auto encoded_data = helper->EncodeData("Mail", data_dict);
  ASSERT_TRUE(encoded_data);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(encoded_data->first)),
            "dd57d9596af52b430ced3d5b52d4e3d5dccfdf3e0572db1dcf526baad311fbd1fc"
            "71e5fa27ff56c350aa531bc129ebdf613b772b6604664f5d8dbe21b85eb0c86447"
            "52e282fcf7fda2a1198d94a0fdc47c09b694e927a40403469fa89f10bbda2b6bac"
            "81575e5745e20d779659dad4d4b9f0967f8d346228028a8675ee5377df");

  // v3 doesn't support array
  helper->SetVersion(EthSignTypedDataHelper::Version::kV3);
  EXPECT_FALSE(helper->EncodeData("Mail", data_dict));
}

TEST(EthSignedTypedDataHelperUnitTest, EncodeField) {
  // types won't matter here
  std::unique_ptr<EthSignTypedDataHelper> helper =
      EthSignTypedDataHelper::Create(base::Value::Dict(),
                                     EthSignTypedDataHelper::Version::kV3);
  ASSERT_TRUE(helper);

  base::Value list(base::Value::Type::LIST);
  list.GetList().Append("hello");
  list.GetList().Append("world");

  // v3 doesn't support array
  EXPECT_FALSE(helper->EncodeField("string[]", list));
  helper->SetVersion(EthSignTypedDataHelper::Version::kV4);

  // invalid arrary type
  EXPECT_FALSE(helper->EncodeField("string[[]]", list));
  // non exist custom array type
  EXPECT_FALSE(helper->EncodeField("Sting[[]]", list));
  EXPECT_FALSE(helper->EncodeField("string[]", base::Value("not list")));
  // v4 handles string array
  {
    auto encoded_field = helper->EncodeField("string[]", list);
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "817f9cf412e48771da9077a54e99b92c920c5a08b06477d97fcc2b64ad9eea8f");
  }
  // not string
  EXPECT_FALSE(helper->EncodeField("string", base::Value(true)));
  // string
  {
    auto encoded_field = helper->EncodeField("string", base::Value("brave"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "e3e90daa6a58ae029a7000d6cc00698775d4b20492e71d5126fada73256f0f26");
  }

  // bytes
  EXPECT_FALSE(helper->EncodeField("bytes", base::Value(true)));
  EXPECT_FALSE(helper->EncodeField("bytes", base::Value("0xx1234")));
  {
    auto encoded_field =
        helper->EncodeField("bytes", base::Value("0x12345678"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "30ca65d5da355227c97ff836c9c6719af9d3835fc6bc72bddc50eeecc1bb2b25");

    // 0x00, 0x0 are handled the same
    encoded_field = helper->EncodeField("bytes", base::Value("0x00"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "bc36789e7a1e281436464229828f817d6612f7b477d66591ff96a9e064bcc98a");
    encoded_field = helper->EncodeField("bytes", base::Value("0x0"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "bc36789e7a1e281436464229828f817d6612f7b477d66591ff96a9e064bcc98a");

    // Empty return
    encoded_field = helper->EncodeField("bytes", base::Value("0x"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");
    encoded_field = helper->EncodeField("bytes", base::Value(""));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");
  }

  // bool
  EXPECT_FALSE(helper->EncodeField("bool", base::Value("not bool")));
  {
    auto encoded_field = helper->EncodeField("bool", base::Value(false));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000000000");
    encoded_field = helper->EncodeField("bool", base::Value(true));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000000001");
  }

  // address
  EXPECT_FALSE(helper->EncodeField("address", base::Value(true)));
  EXPECT_FALSE(helper->EncodeField("address", base::Value("0xx1234")));
  // not 20 bytes
  EXPECT_FALSE(helper->EncodeField(
      "address",
      base::Value("0xaAaAAAAaaAAAaaaAaaAaaaaAAaAaaaaAaAaaAAaABBBb")));
  {
    auto encoded_field = helper->EncodeField(
        "address", base::Value("0xbBbBBBBbbBBBbbbBbbBbbbbBBbBbbbbBbBbbBBbB"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "000000000000000000000000bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
  }

  // bytes1 - bytes32
  EXPECT_FALSE(helper->EncodeField("bytes1", base::Value(true)));
  EXPECT_FALSE(helper->EncodeField("bytes24", base::Value("0xx1234")));
  EXPECT_FALSE(helper->EncodeField("bytesAAA", base::Value("0x1234")));
  EXPECT_FALSE(helper->EncodeField("bytes35", base::Value("0x1234")));
  // exceeds 32 bytes
  EXPECT_FALSE(helper->EncodeField(
      "bytes16", base::Value("0xbBbBBBBbbBBBbbbBbbBbbbbBBbBbbbbBbBbbBBbBdeadbee"
                             "fdeadbeefdeadbeefdeadbeef1234")));
  {
    auto encoded_field =
        helper->EncodeField("bytes5", base::Value("0xdeadbeef"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "deadbeef00000000000000000000000000000000000000000000000000000000");
    encoded_field = helper->EncodeField("bytes18", base::Value("0xdeadbeef"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "deadbeef00000000000000000000000000000000000000000000000000000000");

    // "0x" is treated as empty, for some reason MM doesn't allow empty here
    // but does for "bytes"
    encoded_field = helper->EncodeField("bytes18", base::Value("0x"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000000000");
  }

  // uint8 - uint256
  EXPECT_FALSE(helper->EncodeField("uintA", base::Value(1)));
  EXPECT_FALSE(helper->EncodeField("uint1", base::Value(1)));
  EXPECT_FALSE(helper->EncodeField("uint9", base::Value(1)));
  EXPECT_FALSE(helper->EncodeField("uint264", base::Value(1)));
  EXPECT_FALSE(helper->EncodeField("uint55", base::Value(1)));
  // exceeds 8 bits maximum
  EXPECT_FALSE(helper->EncodeField("uint8", base::Value(256)));
  // exceeds Number.MAX_SAFE_INTEGER
  EXPECT_FALSE(helper->EncodeField(
      "uint256", base::Value(static_cast<double>(9007199254740991) + 1)));
  {
    auto encoded_field = helper->EncodeField("uint8", base::Value(255));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "00000000000000000000000000000000000000000000000000000000000000ff");
    encoded_field = helper->EncodeField("uint32", base::Value(4096));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000001000");

    encoded_field = helper->EncodeField("uint56", base::Value(4096));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000001000");

    encoded_field = helper->EncodeField("uint256", base::Value(65536));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000010000");
  }

  {
    // Encode string values for uint types
    auto encoded_field = helper->EncodeField("uint8", base::Value("255"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "00000000000000000000000000000000000000000000000000000000000000ff");
    encoded_field = helper->EncodeField("uint32", base::Value("4096"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000001000");
    encoded_field = helper->EncodeField("uint56", base::Value("4096"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000001000");
    encoded_field = helper->EncodeField("uint256", base::Value("65536"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000010000");
    // max uint256 in decimal format
    encoded_field = helper->EncodeField(
        "uint256", base::Value("11579208923731619542357098500868790785326998466"
                               "5640564039457584007913129639935"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    // max uint256 in hex format
    encoded_field = helper->EncodeField(
        "uint256", base::Value("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
                               "FFFFFFFFFFFFFFFFFFF"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    // overflow
    EXPECT_FALSE(helper->EncodeField("uint8", base::Value("256")));
    EXPECT_FALSE(helper->EncodeField("uint8", base::Value("0x100")));
    EXPECT_FALSE(helper->EncodeField("uint16", base::Value("65536")));
    EXPECT_FALSE(helper->EncodeField("uint16", base::Value("0x10000")));
    EXPECT_FALSE(helper->EncodeField("uint32", base::Value("4294967296")));
    EXPECT_FALSE(helper->EncodeField("uint32", base::Value("0x100000000")));
    EXPECT_FALSE(
        helper->EncodeField("uint56", base::Value("0x100000000000000")));
    EXPECT_FALSE(
        helper->EncodeField("uint64", base::Value("18446744073709551616")));
    EXPECT_FALSE(
        helper->EncodeField("uint64", base::Value("0x10000000000000000")));
    EXPECT_FALSE(helper->EncodeField(
        "uint128", base::Value("340282366920938463463374607431768211456")));
    EXPECT_FALSE(helper->EncodeField(
        "uint128", base::Value("0x100000000000000000000000000000000")));
    EXPECT_FALSE(helper->EncodeField(
        "uint256", base::Value("11579208923731619542357098500868790785326998466"
                               "56405640394575840079131296399356")));
    EXPECT_FALSE(helper->EncodeField(
        "uint256", base::Value("0x100000000000000000000000000000000000000000000"
                               "00000000000000000000")));

    // Can parse "0x", "0", "", and 0 as 0
    encoded_field = helper->EncodeField("uint32", base::Value("0x"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000000000");
    encoded_field = helper->EncodeField("uint32", base::Value("0"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000000000");
    encoded_field = helper->EncodeField("uint32", base::Value(0));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000000000");
    encoded_field = helper->EncodeField("uint32", base::Value(""));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000000000");
  }

  // int8 - int256
  EXPECT_FALSE(helper->EncodeField("intA", base::Value(1)));
  EXPECT_FALSE(helper->EncodeField("int1", base::Value(1)));
  EXPECT_FALSE(helper->EncodeField("int9", base::Value(1)));
  EXPECT_FALSE(helper->EncodeField("int264", base::Value(1)));
  EXPECT_FALSE(helper->EncodeField("int55", base::Value(1)));
  // exceeds 8 bits maximum
  EXPECT_FALSE(helper->EncodeField("int8", base::Value(128)));
  // exceeds Number.MAX_SAFE_INTEGER
  EXPECT_FALSE(helper->EncodeField(
      "int256", base::Value(static_cast<double>(9007199254740991) + 1)));
  {
    auto encoded_field = helper->EncodeField("int8", base::Value(127));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "000000000000000000000000000000000000000000000000000000000000007f");
    encoded_field = helper->EncodeField("int32", base::Value(4096));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000001000");
    encoded_field = helper->EncodeField("int256", base::Value(65536));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000010000");

    // max int256 in decimal format
    encoded_field = helper->EncodeField(
        "int256", base::Value("578960446186580977117854925043439539266349923328"
                              "20282019728792003956564819967"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    // max int256 in hex format
    encoded_field = helper->EncodeField(
        "int256", base::Value("0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
                              "FFFFFFFFFFFFFFFFFF"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    // min int256 in decimal format
    encoded_field = helper->EncodeField(
        "int256", base::Value("-57896044618658097711785492504343953926634992332"
                              "820282019728792003956564819968"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "8000000000000000000000000000000000000000000000000000000000000000");
    // min int256 in hex format
    encoded_field = helper->EncodeField(
        "int256", base::Value("0x8000000000000000000000000000000000000000000000"
                              "000000000000000000"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "8000000000000000000000000000000000000000000000000000000000000000");

    // overflow
    EXPECT_FALSE(helper->EncodeField("int8", base::Value("128")));
    EXPECT_FALSE(helper->EncodeField("int8", base::Value("-129")));
    EXPECT_FALSE(helper->EncodeField("int8", base::Value("0x100")));
    EXPECT_FALSE(helper->EncodeField("int16", base::Value("32768")));
    EXPECT_FALSE(helper->EncodeField("int16", base::Value("-32769")));
    EXPECT_FALSE(helper->EncodeField("int16", base::Value("0x10000")));
    EXPECT_FALSE(helper->EncodeField("int32", base::Value("2147483648")));
    EXPECT_FALSE(helper->EncodeField("int32", base::Value("-2147483649")));
    EXPECT_FALSE(helper->EncodeField("int32", base::Value("0x100000000")));
    EXPECT_FALSE(
        helper->EncodeField("int56", base::Value("72057594037927935")));
    EXPECT_FALSE(
        helper->EncodeField("int56", base::Value("-72057594037927936")));
    EXPECT_FALSE(
        helper->EncodeField("int64", base::Value("9223372036854775808")));
    EXPECT_FALSE(
        helper->EncodeField("int64", base::Value("-9223372036854775809")));
    EXPECT_FALSE(
        helper->EncodeField("int64", base::Value("0x10000000000000000")));
    EXPECT_FALSE(helper->EncodeField(
        "int128", base::Value("170141183460469231731687303715884105728")));
    EXPECT_FALSE(helper->EncodeField(
        "int128", base::Value("-170141183460469231731687303715884105729")));
    EXPECT_FALSE(helper->EncodeField(
        "int128", base::Value("0x100000000000000000000000000000000")));
    EXPECT_FALSE(helper->EncodeField(
        "int256", base::Value("578960446186580977117854925043439539266349923328"
                              "20282019728792003956564819968")));
    EXPECT_FALSE(helper->EncodeField(
        "int256", base::Value("-57896044618658097711785492504343953926634992332"
                              "820282019728792003956564819969")));
    EXPECT_FALSE(helper->EncodeField(
        "int256", base::Value("0x100000000000000000000000000000000000000000000"
                              "00000000000000000000")));

    // Can parse "0x", "0", "", and 0 as 0
    encoded_field = helper->EncodeField("int32", base::Value("0x"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000000000");
    encoded_field = helper->EncodeField("int32", base::Value("0"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000000000");
    encoded_field = helper->EncodeField("int32", base::Value(0));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000000000");
    encoded_field = helper->EncodeField("int32", base::Value(""));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000000000");
  }

  {
    // Encode string values for int types
    auto encoded_field = helper->EncodeField("int8", base::Value("127"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "000000000000000000000000000000000000000000000000000000000000007f");
    encoded_field = helper->EncodeField("int32", base::Value("4096"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000001000");
    encoded_field = helper->EncodeField("int56", base::Value("4096"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000001000");
    encoded_field = helper->EncodeField("int256", base::Value("65536"));
    ASSERT_TRUE(encoded_field);
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(*encoded_field)),
        "0000000000000000000000000000000000000000000000000000000000010000");
  }
  {  // custom type but not dictionary
    EXPECT_FALSE(helper->EncodeField("Brave123", base::Value(1)));
    EXPECT_FALSE(helper->EncodeField("Brave123", base::Value("123")));
    EXPECT_FALSE(helper->EncodeField("Brave123", base::Value(true)));
    EXPECT_FALSE(helper->EncodeField("Brave123", list));
  }
}

TEST(EthSignedTypedDataHelperUnitTest, GetTypedDataMessageToSign) {
  const std::string types_json(R"({
   "EIP712Domain": [
      { "name": "name", "type": "string" },
      { "name": "version", "type": "string" },
      { "name": "chainId", "type": "uint256" },
      { "name": "verifyingContract", "type": "address" },
    ],
    "Mail": [
        {"name": "from", "type": "Person"},
        {"name": "to", "type": "Person"},
        {"name": "contents", "type": "string"}
    ],
    "Person": [
        {"name": "name", "type": "string"},
        {"name": "wallet", "type": "address"}
    ]})");
  const std::string data_json(R"({
    "from":{"name":"Cow","wallet":"0xCD2a3d9F938E13CD947Ec05AbC7FE734Df8DD826"},
    "to":{"name":"Bob","wallet":"0xbBbBBBBbbBBBbbbBbbBbbbbBBbBbbbbBbBbbBBbB"},
    "contents":"Hello, Bob!"
    })");
  const std::string ds_json(R"({
    "name": "Ether Mail",
    "version": "1",
    "chainId": 1,
    "verifyingContract": "0xCcCCccccCCCCcCCCCCCcCcCccCcCCCcCcccccccC",
  })");

  auto types_value = ParseJson(types_json);
  auto data_value = ParseJson(data_json);
  auto ds_value = ParseJson(ds_json);

  std::unique_ptr<EthSignTypedDataHelper> helper =
      EthSignTypedDataHelper::Create(types_value.GetDict().Clone(),
                                     EthSignTypedDataHelper::Version::kV4);
  ASSERT_TRUE(helper);

  auto ds_hash = helper->HashStruct("EIP712Domain", ds_value.GetDict());
  ASSERT_TRUE(ds_hash);
  auto domain_hash = helper->GetTypedDataDomainHash(ds_value.GetDict());
  ASSERT_TRUE(domain_hash);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(domain_hash->first)),
            "f2cee375fa42b42143804025fc449deafd50cc031ca257e0b194a650a912090f");
  auto primary_hash =
      helper->GetTypedDataPrimaryHash("Mail", data_value.GetDict());
  ASSERT_TRUE(primary_hash);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(primary_hash->first)),
            "c52c0ee5d84264471806290a3f2c4cecfc5490626bf912d01f240d7a274b371e");
  auto message_to_sign = helper->GetTypedDataMessageToSign(domain_hash->first,
                                                           primary_hash->first);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(message_to_sign)),
            "be609aee343fb3c4b28e1df9e632fca64fcfaede20f02e86244efddf30957bd2");
}

}  // namespace brave_wallet
