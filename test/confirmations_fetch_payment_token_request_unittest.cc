/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <memory>

#include "confirmations_client_mock.h"
#include "bat-native-confirmations/src/confirmations_impl.h"
#include "bat-native-confirmations/src/fetch_payment_token_request.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

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
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case
};

TEST_F(ConfirmationsFetchPaymentTokenRequestTest, BuildUrl) {
  // Arrange
  std::string confirmation_id = "c7f8c42d-6768-4dd7-8dc6-612cbba3ec21";

  // Act
  auto url = request_->BuildUrl(confirmation_id);

  // Assert
  std::string expected_url = "https://ads-serve.bravesoftware.com/v1/confirmation/c7f8c42d-6768-4dd7-8dc6-612cbba3ec21/paymentToken";  // NOLINT
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
