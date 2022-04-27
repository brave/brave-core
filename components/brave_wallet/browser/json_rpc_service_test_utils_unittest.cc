/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/json_rpc_service_test_utils.h"

#include "base/json/json_reader.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {
std::string ParseRpcJsonResult(const std::string& json) {
  absl::optional<base::Value> value = base::JSONReader::Read(json);
  EXPECT_TRUE(value);
  EXPECT_TRUE(value->is_dict());
  EXPECT_THAT(*value,
              base::test::DictionaryHasValue("jsonrpc", base::Value("2.0")));
  EXPECT_THAT(*value, base::test::DictionaryHasValue("id", base::Value(1)));
  auto* result = value->GetDict().FindString("result");
  EXPECT_TRUE(result);
  EXPECT_THAT(*result, testing::StartsWith("0x"));
  return result->substr(2);
}

std::string DecodeTuple1(const std::string& data) {
  std::string head =
      "0000000000000000000000000000000000000000000000000000000000000020";
  EXPECT_THAT(data, testing::StartsWith(head));
  return data.substr(head.size());
}
}  // namespace

TEST(JsonRpcServiceTestUtils, MakeJsonRpcStringArrayResponse) {
  auto json = MakeJsonRpcStringArrayResponse({"one", "two", "three"});

  auto* expected =
      R"({"jsonrpc":"2.0", "id":1, "result":"0x)"
      R"(0000000000000000000000000000000000000000000000000000000000000020)"
      R"(0000000000000000000000000000000000000000000000000000000000000003)"
      R"(0000000000000000000000000000000000000000000000000000000000000060)"
      R"(00000000000000000000000000000000000000000000000000000000000000a0)"
      R"(00000000000000000000000000000000000000000000000000000000000000e0)"
      R"(0000000000000000000000000000000000000000000000000000000000000003)"
      R"(6f6e650000000000000000000000000000000000000000000000000000000000)"
      R"(0000000000000000000000000000000000000000000000000000000000000003)"
      R"(74776f0000000000000000000000000000000000000000000000000000000000)"
      R"(0000000000000000000000000000000000000000000000000000000000000005)"
      R"(7468726565000000000000000000000000000000000000000000000000000000"})";
  EXPECT_EQ(expected, json);

  std::vector<std::string> decoded;
  EXPECT_TRUE(
      DecodeStringArray(DecodeTuple1(ParseRpcJsonResult(json)), &decoded));
  EXPECT_THAT(decoded, testing::ElementsAreArray({"one", "two", "three"}));
}

TEST(JsonRpcServiceTestUtils, MakeJsonRpcStringArrayResponse_Empty) {
  auto json = MakeJsonRpcStringArrayResponse({});

  auto* expected =
      R"({"jsonrpc":"2.0", "id":1, "result":"0x)"
      R"(0000000000000000000000000000000000000000000000000000000000000020)"
      R"(0000000000000000000000000000000000000000000000000000000000000000)"
      R"(0000000000000000000000000000000000000000000000000000000000000000"})";
  EXPECT_EQ(expected, json);

  std::vector<std::string> decoded;
  EXPECT_TRUE(
      DecodeStringArray(DecodeTuple1(ParseRpcJsonResult(json)), &decoded));
  EXPECT_THAT(decoded, testing::IsEmpty());
}

TEST(JsonRpcServiceTestUtils,
     MakeJsonRpcStringArrayResponse_UnstoppableDomainsDNS) {
  std::vector<std::string> records = {
      "QmWrdNJWMbvRxxzLhojVKaBDswS4KNVM7LvjsN7QbDrvka",
      "QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR",
      "",
      "",
      "https://fallback1.test.com",
      "https://fallback2.test.com"};

  auto json = MakeJsonRpcStringArrayResponse(records);

  auto* expected =
      R"({"jsonrpc":"2.0", "id":1, "result":"0x)"
      // offset for array
      R"(0000000000000000000000000000000000000000000000000000000000000020)"
      // count for array
      R"(0000000000000000000000000000000000000000000000000000000000000006)"
      // offsets for array elements
      R"(00000000000000000000000000000000000000000000000000000000000000c0)"
      R"(0000000000000000000000000000000000000000000000000000000000000120)"
      R"(0000000000000000000000000000000000000000000000000000000000000180)"
      R"(00000000000000000000000000000000000000000000000000000000000001a0)"
      R"(00000000000000000000000000000000000000000000000000000000000001c0)"
      R"(0000000000000000000000000000000000000000000000000000000000000200)"
      // count for "QmWrdNJWMbvRxxzLhojVKaBDswS4KNVM7LvjsN7QbDrvka"
      R"(000000000000000000000000000000000000000000000000000000000000002e)"
      // encoding for "QmWrdNJWMbvRxxzLhojVKaBDswS4KNVM7LvjsN7QbDrvka"
      R"(516d5772644e4a574d62765278787a4c686f6a564b614244737753344b4e564d)"
      R"(374c766a734e3751624472766b61000000000000000000000000000000000000)"
      // count for "QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR"
      R"(000000000000000000000000000000000000000000000000000000000000002e)"
      // encoding for "QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR"
      R"(516d6257717842454b433350387471734b633938786d574e7a727a4474524c4d)"
      R"(694d504c387742755447734d6e52000000000000000000000000000000000000)"
      // count for empty dns.A
      R"(0000000000000000000000000000000000000000000000000000000000000000)"
      // count for empty dns.AAAA
      R"(0000000000000000000000000000000000000000000000000000000000000000)"
      // count for "https://fallback1.test.com"
      R"(000000000000000000000000000000000000000000000000000000000000001a)"
      // encoding for "https://fallback1.test.com"
      R"(68747470733a2f2f66616c6c6261636b312e746573742e636f6d000000000000)"
      // count for "https://fallback2.test.com"
      R"(000000000000000000000000000000000000000000000000000000000000001a)"
      // encoding for "https://fallback2.test.com"
      R"(68747470733a2f2f66616c6c6261636b322e746573742e636f6d000000000000"})";

  EXPECT_EQ(expected, json);

  std::vector<std::string> decoded;
  EXPECT_TRUE(
      DecodeStringArray(DecodeTuple1(ParseRpcJsonResult(json)), &decoded));
  EXPECT_THAT(decoded, testing::ElementsAreArray(records));
}

TEST(JsonRpcServiceTestUtils, MakeJsonRpcStringResponse) {
  auto json = MakeJsonRpcStringResponse("a");

  auto* expected =
      R"({"jsonrpc":"2.0", "id":1, "result":"0x)"
      R"(0000000000000000000000000000000000000000000000000000000000000020)"
      R"(0000000000000000000000000000000000000000000000000000000000000001)"
      R"(6100000000000000000000000000000000000000000000000000000000000000"})";
  EXPECT_EQ(expected, json);

  std::string decoded;
  EXPECT_TRUE(
      DecodeString(0, DecodeTuple1(ParseRpcJsonResult(json)), &decoded));
  EXPECT_EQ(decoded, "a");
}

TEST(JsonRpcServiceTestUtils,
     MakeJsonRpcStringArrayResponse_UnstoppableDomainsEthAddr) {
  auto json =
      MakeJsonRpcStringResponse("0x3a2f3f7aab82d69036763cfd3f755975f84496e6");

  auto* expected =
      R"({"jsonrpc":"2.0", "id":1, "result":"0x)"
      R"(0000000000000000000000000000000000000000000000000000000000000020)"
      R"(000000000000000000000000000000000000000000000000000000000000002a)"
      R"(3078336132663366376161623832643639303336373633636664336637353539)"
      R"(3735663834343936653600000000000000000000000000000000000000000000"})";
  EXPECT_EQ(expected, json);

  std::string decoded;
  EXPECT_TRUE(
      DecodeString(0, DecodeTuple1(ParseRpcJsonResult(json)), &decoded));
  EXPECT_EQ(decoded, "0x3a2f3f7aab82d69036763cfd3f755975f84496e6");
}

TEST(JsonRpcServiceTestUtils, MakeJsonRpcStringResponse_Empty) {
  auto json = MakeJsonRpcStringResponse("");

  auto* expected =
      R"({"jsonrpc":"2.0", "id":1, "result":"0x)"
      R"(0000000000000000000000000000000000000000000000000000000000000020)"
      R"(0000000000000000000000000000000000000000000000000000000000000000"})";
  EXPECT_EQ(expected, json);

  std::string decoded;
  EXPECT_TRUE(
      DecodeString(0, DecodeTuple1(ParseRpcJsonResult(json)), &decoded));
  EXPECT_EQ(decoded, "");
}

TEST(JsonRpcServiceTestUtils, MakeJsonRpcErrorResponse) {
  auto json = MakeJsonRpcErrorResponse(123, "Error!");
  absl::optional<base::Value> value = base::JSONReader::Read(json);
  ASSERT_TRUE(value);
  ASSERT_TRUE(value->is_dict());
  EXPECT_EQ("2.0", *value->GetDict().FindString("jsonrpc"));
  EXPECT_EQ(1, *value->GetDict().FindInt("id"));
  EXPECT_EQ(123, *value->GetDict().FindIntByDottedPath("error.code"));
  EXPECT_EQ("Error!",
            *value->GetDict().FindStringByDottedPath("error.message"));
}

}  // namespace brave_wallet
