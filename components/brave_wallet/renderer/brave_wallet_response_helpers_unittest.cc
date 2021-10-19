/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/renderer/brave_wallet_response_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(BraveWalletResponseHelpersTest, GetProviderErrorDictionary) {
  ProviderErrors code = ProviderErrors::kUnsupportedMethod;
  std::string message = "HTTP Status code: " + base::NumberToString(400);
  std::unique_ptr<base::Value> result =
      GetProviderErrorDictionary(code, message);
  ASSERT_TRUE(result != nullptr);
  const base::Value* result_code = result->FindKey("code");
  const base::Value* result_message = result->FindKey("message");
  EXPECT_TRUE(result_code && result_code->is_int());
  EXPECT_TRUE(result_message && result_message->is_string());

  EXPECT_EQ(result_code->GetInt(), static_cast<int>(code));
  EXPECT_EQ(result_message->GetString(), message);
}

TEST(BraveWalletResponseHelpersTest,
     GetProviderRequestReturnFromEthJsonResponseError) {
  std::string response =
      "{\"jsonrpc\":\"2.0\",\"id\":2025678280,\"error\":{\"code\":-32601,"
      "\"message\":\"The method eth_accountsq does not exist/is not "
      "available\"}}";
  bool reject = false;
  std::unique_ptr<base::Value> result =
      GetProviderRequestReturnFromEthJsonResponse(200, response, &reject);
  ASSERT_TRUE(result != nullptr);
  EXPECT_TRUE(reject);
  const base::Value* result_code = result->FindKey("code");
  const base::Value* result_message = result->FindKey("message");
  EXPECT_TRUE(result_code && result_code->is_int());
  EXPECT_TRUE(result_message && result_message->is_string());

  EXPECT_EQ(result_code->GetInt(), -32601);
  EXPECT_EQ(result_message->GetString(),
            "The method eth_accountsq does not exist/is not available");
}

TEST(BraveWalletResponseHelpersTest,
     GetProviderRequestReturnFromEthJsonResponseErrorHTTP) {
  std::string response = "";
  bool reject = false;
  std::unique_ptr<base::Value> result =
      GetProviderRequestReturnFromEthJsonResponse(400, response, &reject);
  ASSERT_TRUE(result != nullptr);
  EXPECT_TRUE(reject);
  const base::Value* result_code = result->FindKey("code");
  const base::Value* result_message = result->FindKey("message");
  EXPECT_TRUE(result_code && result_code->is_int());
  EXPECT_TRUE(result_message && result_message->is_string());
  EXPECT_EQ(result_code->GetInt(), (int)ProviderErrors::kUnsupportedMethod);
  EXPECT_EQ(result_message->GetString(), "HTTP Status code: 400");
}

TEST(BraveWalletResponseHelpersTest,
     GetProviderRequestReturnFromEthJsonResponseSuccess) {
  std::string response =
      "{\"jsonrpc\":\"2.0\",\"id\":2025678280,\"result\":\"0xbb4323\"}";
  bool reject = false;
  std::unique_ptr<base::Value> result =
      GetProviderRequestReturnFromEthJsonResponse(200, response, &reject);

  ASSERT_TRUE(result != nullptr);
  EXPECT_TRUE(result->is_string());
  EXPECT_FALSE(reject);
  EXPECT_EQ(result->GetString(), "0xbb4323");
}

TEST(BraveWalletResponseHelpersTest, ToProviderResponseEmpty) {
  std::unique_ptr<base::Value> result =
      ToProviderResponse(base::Value(), nullptr, nullptr);

  ASSERT_TRUE(result != nullptr);
  EXPECT_TRUE(result->is_dict());
  EXPECT_EQ(*result->FindPath("id"), base::Value());
  EXPECT_EQ(result->FindPath("jsonrpc")->GetString(), "2.0");
  EXPECT_FALSE(result->FindPath("result"));
  EXPECT_FALSE(result->FindPath("error"));
}

TEST(BraveWalletResponseHelpersTest, ToProviderResponseSuccess) {
  base::Value value("test");
  std::unique_ptr<base::Value> result =
      ToProviderResponse(base::Value(2), &value, nullptr);

  ASSERT_TRUE(result != nullptr);
  EXPECT_TRUE(result->is_dict());
  EXPECT_EQ(*result->FindPath("id"), base::Value(2));
  EXPECT_EQ(result->FindPath("jsonrpc")->GetString(), "2.0");
  EXPECT_EQ(result->FindPath("result")->GetString(), value.GetString());
  EXPECT_FALSE(result->FindPath("error"));
}

TEST(BraveWalletResponseHelpersTest, ToProviderResponseError) {
  base::Value value("test");
  base::Value error("error");
  std::unique_ptr<base::Value> result =
      ToProviderResponse(base::Value("hi"), &value, &error);

  ASSERT_TRUE(result != nullptr);
  EXPECT_TRUE(result->is_dict());
  EXPECT_EQ(*result->FindPath("id"), base::Value("hi"));
  EXPECT_EQ(result->FindPath("jsonrpc")->GetString(), "2.0");
  EXPECT_EQ(result->FindPath("result")->GetString(), value.GetString());
  EXPECT_TRUE(result->FindPath("error")->Equals(&error));
}

}  // namespace brave_wallet
