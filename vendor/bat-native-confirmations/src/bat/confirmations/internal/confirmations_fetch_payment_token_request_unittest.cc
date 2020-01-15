/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <memory>

#include "bat/confirmations/internal/confirmations_client_mock.h"
#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/fetch_payment_token_request.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

using std::placeholders::_1;

namespace confirmations {

class ConfirmationsFetchPaymentTokenRequestTest : public ::testing::Test {
 protected:
  std::unique_ptr<MockConfirmationsClient> mock_confirmations_client_;
  std::unique_ptr<ConfirmationsImpl> confirmations_;

  std::unique_ptr<FetchPaymentTokenRequest> request_;

  ConfirmationsFetchPaymentTokenRequestTest() :
      mock_confirmations_client_(std::make_unique<MockConfirmationsClient>()),
      confirmations_(std::make_unique<ConfirmationsImpl>(
          mock_confirmations_client_.get())),
      request_(std::make_unique<FetchPaymentTokenRequest>()) {
    // You can do set-up work for each test here
  }

  ~ConfirmationsFetchPaymentTokenRequestTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)
    auto callback = std::bind(
        &ConfirmationsFetchPaymentTokenRequestTest::OnInitialize, this, _1);
    confirmations_->Initialize(callback);
  }

  void OnInitialize(const bool success) {
    EXPECT_EQ(true, success);
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case
};

TEST_F(ConfirmationsFetchPaymentTokenRequestTest, BuildUrl) {
  // Arrange
  std::string confirmation_id = "546fe7b0-5047-4f28-a11c-81f14edcf0f6";

  // Act
  auto url = request_->BuildUrl(confirmation_id);

  // Assert
  std::string expected_url = "https://ads-serve.bravesoftware.com/v1/confirmation/546fe7b0-5047-4f28-a11c-81f14edcf0f6/paymentToken";  // NOLINT
  EXPECT_EQ(expected_url, url);
}

TEST_F(ConfirmationsFetchPaymentTokenRequestTest, GetMethod) {
  // Arrange

  // Act
  auto method = request_->GetMethod();

  // Assert
  EXPECT_EQ(URLRequestMethod::GET, method);
}

}  // namespace confirmations
