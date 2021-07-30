/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/renderer/brave_wallet_response_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(BraveWalletResponseHelpersTest, ResponseCode400) {
  ProviderErrors code = ProviderErrors::kUnsupportedMethod;
  std::string message = "HTTP Status code: " + base::NumberToString(400);
  std::unique_ptr<base::Value> result = FormProviderResponse(code, message);
  ASSERT_TRUE(result != nullptr);
  const base::Value* result_code = result->FindKey("code");
  const base::Value* result_message = result->FindKey("message");
  ASSERT_TRUE(result_code && result_code->is_int());
  ASSERT_TRUE(result_message && result_message->is_string());

  ASSERT_EQ(result_code->GetInt(), static_cast<int>(code));
  ASSERT_EQ(result_message->GetString(), message);
}

TEST(BraveWalletResponseHelpersTest, ErrorResponse) {
  std::string response =
      "{\"jsonrpc\":\"2.0\",\"id\":2025678280,\"error\":{\"code\":-32601,"
      "\"message\":\"The method eth_accountsq does not exist/is not "
      "available\"}}";
  bool reject = false;
  std::unique_ptr<base::Value> result =
      FormProviderResponse(response, false, &reject);

  ASSERT_TRUE(result != nullptr);
  ASSERT_TRUE(reject);
  const base::Value* result_code = result->FindKey("code");
  const base::Value* result_message = result->FindKey("message");
  ASSERT_TRUE(result_code && result_code->is_int());
  ASSERT_TRUE(result_message && result_message->is_string());

  ASSERT_EQ(result_code->GetInt(), -32601);
  ASSERT_EQ(result_message->GetString(),
            "The method eth_accountsq does not exist/is not available");
}

TEST(BraveWalletResponseHelpersTest, CorrectResultResponse) {
  std::string response =
      "{\"jsonrpc\":\"2.0\",\"id\":2025678280,\"result\":\"0xbb4323\"}";
  bool reject = false;
  std::unique_ptr<base::Value> result =
      FormProviderResponse(response, false, &reject);

  ASSERT_TRUE(result != nullptr);
  ASSERT_TRUE(result->is_string());
  ASSERT_FALSE(reject);
  ASSERT_EQ(result->GetString(), "0xbb4323");
}

}  // namespace brave_wallet
