/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/get_signed_tokens_request.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"
#include "bat/confirmations/wallet_info.h"

// npm run test -- brave_unit_tests --filter=BatConfirmations*

namespace confirmations {

TEST(BatConfirmationsGetSignedTokensRequestTest,
    BuildUrl) {
  // Arrange
  WalletInfo wallet;
  wallet.payment_id = "d4ed0af0-bfa9-464b-abd7-67b29d891b8b";
  wallet.private_key =
      "e9b1ab4f44d39eb04323411eed0b5a2ceedff01264474f86e29c707a56615650"
      "33cea0085cfd551faa170c1dd7f6daaa903cdd3138d61ed5ab2845e224d58144";

  const std::string nonce = "716c3381-66e6-46e4-962f-15d01455b5b9";

  const GetSignedTokensRequest request;

  // Act
  const std::string url = request.BuildUrl(wallet, nonce);

  // Assert
  const std::string expected_url = R"(https://ads-serve.bravesoftware.com/v1/confirmation/token/d4ed0af0-bfa9-464b-abd7-67b29d891b8b?nonce=716c3381-66e6-46e4-962f-15d01455b5b9)";

  EXPECT_EQ(expected_url, url);
}

TEST(BatConfirmationsGetSignedTokensRequestTest,
    GetMethod) {
  // Arrange
  const GetSignedTokensRequest request;

  // Act
  const URLRequestMethod method = request.GetMethod();

  // Assert
  EXPECT_EQ(URLRequestMethod::GET, method);
}

}  // namespace confirmations
