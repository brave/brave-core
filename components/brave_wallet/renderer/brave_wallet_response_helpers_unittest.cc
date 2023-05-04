/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet_response_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJson;

namespace brave_wallet {

TEST(BraveWalletResponseHelpersTest, GetProviderErrorDictionary) {
  mojom::ProviderError code = mojom::ProviderError::kUnsupportedMethod;
  std::string message = "HTTP Status code: " + base::NumberToString(400);
  base::Value result = GetProviderErrorDictionary(code, message);
  ASSERT_TRUE(result.is_dict());
  auto& dict = result.GetDict();
  const auto result_code = dict.FindInt("code");
  const auto* result_message = dict.FindString("message");
  EXPECT_TRUE(result_code);
  EXPECT_TRUE(result_message);

  EXPECT_EQ(*result_code, static_cast<int>(code));
  EXPECT_EQ(*result_message, message);
}

TEST(BraveWalletResponseHelpersTest,
     GetProviderRequestReturnFromEthJsonResponseError) {
  std::string response =
      "{\"jsonrpc\":\"2.0\",\"id\":2025678280,\"error\":{\"code\":-32601,"
      "\"message\":\"The method eth_accountsq does not exist/is not "
      "available\"}}";
  bool reject = false;
  base::Value result = GetProviderRequestReturnFromEthJsonResponse(
      200, ParseJson(response), &reject);
  EXPECT_TRUE(reject);
  ASSERT_TRUE(result.is_dict());
  auto& dict = result.GetDict();
  const auto result_code = dict.FindInt("code");
  const auto* result_message = dict.FindString("message");
  EXPECT_TRUE(result_code);
  EXPECT_TRUE(result_message);

  EXPECT_EQ(*result_code, -32601);
  EXPECT_EQ(*result_message,
            "The method eth_accountsq does not exist/is not available");
}

TEST(BraveWalletResponseHelpersTest,
     GetProviderRequestReturnFromEthJsonResponseErrorHTTP) {
  bool reject = false;
  base::Value result =
      GetProviderRequestReturnFromEthJsonResponse(400, base::Value(), &reject);
  ASSERT_TRUE(result.is_dict());
  auto& dict = result.GetDict();
  const auto result_code = dict.FindInt("code");
  const auto* result_message = dict.FindString("message");
  EXPECT_TRUE(result_code);
  EXPECT_TRUE(result_message);
  EXPECT_EQ(*result_code, (int)mojom::ProviderError::kUnsupportedMethod);
  EXPECT_EQ(*result_message, "HTTP Status code: 400");
}

TEST(BraveWalletResponseHelpersTest,
     GetProviderRequestReturnFromEthJsonResponseSuccess) {
  std::string response =
      "{\"jsonrpc\":\"2.0\",\"id\":2025678280,\"result\":\"0xbb4323\"}";
  bool reject = false;
  auto result = GetProviderRequestReturnFromEthJsonResponse(
      200, ParseJson(response), &reject);

  EXPECT_TRUE(result.is_string());
  EXPECT_FALSE(reject);
  EXPECT_EQ(result.GetString(), "0xbb4323");
}

TEST(BraveWalletResponseHelpersTest, ToProviderResponseEmpty) {
  auto result = ToProviderResponse(base::Value(), nullptr, nullptr);

  ASSERT_TRUE(result.is_dict());
  auto& dict = result.GetDict();
  EXPECT_EQ(*dict.Find("id"), base::Value());
  EXPECT_EQ(*dict.FindString("jsonrpc"), "2.0");
  EXPECT_FALSE(dict.Find("result"));
  EXPECT_FALSE(dict.Find("error"));
}

TEST(BraveWalletResponseHelpersTest, ToProviderResponseSuccess) {
  base::Value value("test");
  base::Value result = ToProviderResponse(base::Value(2), &value, nullptr);

  ASSERT_TRUE(result.is_dict());
  auto& dict = result.GetDict();
  EXPECT_EQ(*dict.Find("id"), base::Value(2));
  EXPECT_EQ(*dict.FindString("jsonrpc"), "2.0");
  EXPECT_EQ(*dict.FindString("result"), value.GetString());
  EXPECT_FALSE(dict.Find("error"));
}

TEST(BraveWalletResponseHelpersTest, ToProviderResponseError) {
  base::Value value("test");
  base::Value error("error");
  base::Value result = ToProviderResponse(base::Value("hi"), &value, &error);

  ASSERT_TRUE(result.is_dict());
  auto& dict = result.GetDict();
  EXPECT_EQ(*dict.Find("id"), base::Value("hi"));
  EXPECT_EQ(*dict.FindString("jsonrpc"), "2.0");
  EXPECT_EQ(*dict.FindString("result"), value.GetString());
  EXPECT_EQ(*dict.Find("error"), error);
}

}  // namespace brave_wallet
