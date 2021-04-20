/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/brave_wallet_response_helpers.h"
#include "base/strings/string_number_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(BraveWalletResponseHelpersTest, ResponseCode400) {
  ProviderErrors code = ProviderErrors::kUnsupportedMethod;
  std::string message = "HTTP Status code: " + base::NumberToString(400);
  ASSERT_EQ(FormProviderResponse(code, message),
            "{\"code\":4200,\"message\":\"HTTP Status code: 400\"}");
}

TEST(BraveWalletResponseHelpersTest, ErrorResponse) {
  std::string response =
      "{\"jsonrpc\":\"2.0\",\"id\":2025678280,\"error\":{\"code\":-32601,"
      "\"message\":\"The method eth_accountsq does not exist/is not "
      "available\"}}";
  bool reject = false;
  ASSERT_EQ(FormProviderResponse(response, &reject),
            "{\"code\":-32601,\"message\":\"The method eth_accountsq does not "
            "exist/is not available\"}");
  ASSERT_TRUE(reject);
}

TEST(BraveWalletResponseHelpersTest, CorrectResultResponse) {
  std::string response =
      "{\"jsonrpc\":\"2.0\",\"id\":2025678280,\"result\":\"0xbb4323\"}";
  bool reject = false;
  ASSERT_EQ(FormProviderResponse(response, &reject), "\"0xbb4323\"");
  ASSERT_FALSE(reject);
}

}  // namespace brave_wallet
