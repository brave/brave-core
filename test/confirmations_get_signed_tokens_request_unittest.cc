/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "confirmations_client_mock.h"
#include "brave/vendor/bat-native-confirmations/src/confirmations_impl.h"
#include "brave/vendor/bat-native-confirmations/src/get_signed_tokens_request.h"
#include "brave/vendor/bat-native-confirmations/include/bat/confirmations/wallet_info.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

namespace confirmations {

class ConfirmationsGetSignedTokensRequestTest : public ::testing::Test {
 protected:
  MockConfirmationsClient* mock_confirmations_client_;
  ConfirmationsImpl* confirmations_;

  GetSignedTokensRequest* request_;

  ConfirmationsGetSignedTokensRequestTest() :
      mock_confirmations_client_(new MockConfirmationsClient()),
      confirmations_(new ConfirmationsImpl(mock_confirmations_client_)),
      request_(new GetSignedTokensRequest()) {
    // You can do set-up work for each test here
  }

  ~ConfirmationsGetSignedTokensRequestTest() override {
    // You can do clean-up work that doesn't throw exceptions here
    delete request_;

    delete confirmations_;
    delete mock_confirmations_client_;
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
};

TEST_F(ConfirmationsGetSignedTokensRequestTest, BuildUrl) {
  // Arrange
  WalletInfo wallet_info;
  wallet_info.payment_id = "e7fcf220-d3f4-4111-a0b2-6157d0347567";
  wallet_info.public_key = "3fc8ff3b121e7b7875750d26eaba6f06a3b06d96cf6b2fb898323917e7be9d16e255a4a6f7eb8647428f727c0d4e1958bd8e69a984eee38514d1e483aab27edf";  // NOLINT

  std::string nonce = "8561a644-6f42-49be-a2f4-4bc69dc87a27";

  // Act
  auto url = request_->BuildUrl(wallet_info, nonce);

  // Assert
  std::string expected_url = "https://ads-serve.bravesoftware.com/v1/confirmation/token/e7fcf220-d3f4-4111-a0b2-6157d0347567?nonce=8561a644-6f42-49be-a2f4-4bc69dc87a27";  // NOLINT
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
