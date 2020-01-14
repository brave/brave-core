/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <memory>

#include "bat/confirmations/wallet_info.h"

#include "bat/confirmations/internal/confirmations_client_mock.h"
#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/get_signed_tokens_request.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

using std::placeholders::_1;

namespace confirmations {

class ConfirmationsGetSignedTokensRequestTest : public ::testing::Test {
 protected:
  std::unique_ptr<MockConfirmationsClient> mock_confirmations_client_;
  std::unique_ptr<ConfirmationsImpl> confirmations_;

  std::unique_ptr<GetSignedTokensRequest> request_;

  ConfirmationsGetSignedTokensRequestTest() :
      mock_confirmations_client_(std::make_unique<MockConfirmationsClient>()),
      confirmations_(std::make_unique<ConfirmationsImpl>(
          mock_confirmations_client_.get())),
      request_(std::make_unique<GetSignedTokensRequest>()) {
    // You can do set-up work for each test here
  }

  ~ConfirmationsGetSignedTokensRequestTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)
    auto callback = std::bind(
        &ConfirmationsGetSignedTokensRequestTest::OnInitialize, this, _1);
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

TEST_F(ConfirmationsGetSignedTokensRequestTest, BuildUrl) {
  // Arrange
  WalletInfo wallet_info;
  wallet_info.payment_id = "d4ed0af0-bfa9-464b-abd7-67b29d891b8b";
  wallet_info.private_key = "e9b1ab4f44d39eb04323411eed0b5a2ceedff01264474f86e29c707a5661565033cea0085cfd551faa170c1dd7f6daaa903cdd3138d61ed5ab2845e224d58144";  // NOLINT

  std::string nonce = "716c3381-66e6-46e4-962f-15d01455b5b9";

  // Act
  auto url = request_->BuildUrl(wallet_info, nonce);

  // Assert
  std::string expected_url = "https://ads-serve.bravesoftware.com/v1/confirmation/token/d4ed0af0-bfa9-464b-abd7-67b29d891b8b?nonce=716c3381-66e6-46e4-962f-15d01455b5b9";  // NOLINT
  EXPECT_EQ(expected_url, url);
}

TEST_F(ConfirmationsGetSignedTokensRequestTest, GetMethod) {
  // Arrange

  // Act
  auto method = request_->GetMethod();

  // Assert
  EXPECT_EQ(URLRequestMethod::GET, method);
}

}  // namespace confirmations
