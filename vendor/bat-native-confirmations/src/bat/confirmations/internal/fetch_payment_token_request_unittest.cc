/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/fetch_payment_token_request.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatConfirmations*

namespace confirmations {

TEST(BatConfirmationsFetchPaymentTokenRequestTest,
    BuildUrl) {
  // Arrange
  const std::string confirmation_id = "546fe7b0-5047-4f28-a11c-81f14edcf0f6";

  const FetchPaymentTokenRequest request;

  // Act
  const std::string url = request.BuildUrl(confirmation_id);

  // Assert
  const std::string expected_url = R"(https://ads-serve.bravesoftware.com/v1/confirmation/546fe7b0-5047-4f28-a11c-81f14edcf0f6/paymentToken)";

  EXPECT_EQ(expected_url, url);
}

TEST(BatConfirmationsFetchPaymentTokenRequestTest,
    GetMethod) {
  // Arrange
  const FetchPaymentTokenRequest request;

  // Act
  const URLRequestMethod method = request.GetMethod();

  // Assert
  EXPECT_EQ(URLRequestMethod::GET, method);
}

}  // namespace confirmations
