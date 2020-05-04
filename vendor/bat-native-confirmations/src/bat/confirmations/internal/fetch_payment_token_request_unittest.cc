/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/fetch_payment_token_request.h"

#include <memory>
#include <string>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatConfirmations*

namespace confirmations {

class BatConfirmationsFetchPaymentTokenRequestTest : public ::testing::Test {
 protected:
  BatConfirmationsFetchPaymentTokenRequestTest()
      : request_(std::make_unique<FetchPaymentTokenRequest>()) {
    // You can do set-up work for each test here
  }

  ~BatConfirmationsFetchPaymentTokenRequestTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  std::unique_ptr<FetchPaymentTokenRequest> request_;
};

TEST_F(BatConfirmationsFetchPaymentTokenRequestTest,
    BuildUrl) {
  // Arrange
  const std::string confirmation_id = "546fe7b0-5047-4f28-a11c-81f14edcf0f6";

  // Act
  const std::string url = request_->BuildUrl(confirmation_id);

  // Assert
  const std::string expected_url = "https://ads-serve.bravesoftware.com/v1/confirmation/546fe7b0-5047-4f28-a11c-81f14edcf0f6/paymentToken";  // NOLINT
  EXPECT_EQ(expected_url, url);
}

TEST_F(BatConfirmationsFetchPaymentTokenRequestTest,
    GetMethod) {
  // Arrange

  // Act
  const URLRequestMethod method = request_->GetMethod();

  // Assert
  EXPECT_EQ(URLRequestMethod::GET, method);
}

}  // namespace confirmations
