/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <memory>

#include "bat/confirmations/wallet_info.h"

#include "bat/confirmations/internal/confirmations_client_mock.h"
#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/create_confirmation_request.h"
#include "bat/confirmations/internal/security_helper.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

using std::placeholders::_1;

namespace confirmations {

class ConfirmationsCreateConfirmationRequestTest : public ::testing::Test {
 protected:
  std::unique_ptr<MockConfirmationsClient> mock_confirmations_client_;
  std::unique_ptr<ConfirmationsImpl> confirmations_;

  std::unique_ptr<CreateConfirmationRequest> request_;

  ConfirmationsCreateConfirmationRequestTest() :
      mock_confirmations_client_(std::make_unique<MockConfirmationsClient>()),
      confirmations_(std::make_unique<ConfirmationsImpl>(
          mock_confirmations_client_.get())),
      request_(std::make_unique<CreateConfirmationRequest>()) {
    // You can do set-up work for each test here
  }

  ~ConfirmationsCreateConfirmationRequestTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)
    auto callback = std::bind(
        &ConfirmationsCreateConfirmationRequestTest::OnInitialize, this, _1);
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

TEST_F(ConfirmationsCreateConfirmationRequestTest, BuildUrl) {
  // Arrange
  std::string confirmation_id = "d990ed8d-d739-49fb-811b-c2e02158fb60";
  std::string credential = "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=";  // NOLINT

  // Act
  auto url = request_->BuildUrl(confirmation_id, credential);

  // Assert
  std::string expected_url = "https://ads-serve.bravesoftware.com/v1/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=";  // NOLINT
  EXPECT_EQ(expected_url, url);
}

TEST_F(ConfirmationsCreateConfirmationRequestTest, GetMethod) {
  // Arrange

  // Act
  auto method = request_->GetMethod();

  // Assert
  EXPECT_EQ(URLRequestMethod::POST, method);
}

TEST_F(ConfirmationsCreateConfirmationRequestTest, BuildBody_Viewed) {
  // Arrange
  std::string creative_instance_id = "546fe7b0-5047-4f28-a11c-81f14edcf0f6";

  std::string blinded_token_base64 =
      "PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=";
  auto blinded_token = BlindedToken::decode_base64(blinded_token_base64);

  auto payload = request_->CreateConfirmationRequestDTO(creative_instance_id,
      blinded_token, ConfirmationType::VIEW);

  // Act
  auto body = request_->BuildBody(payload);

  // Assert
  std::string expected_body = R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"type":"view"})";  // NOLINT
  EXPECT_EQ(expected_body, body);
}

TEST_F(ConfirmationsCreateConfirmationRequestTest, HeadersCount) {
  // Arrange

  // Act
  auto headers = request_->BuildHeaders();

  // Assert
  auto count = headers.size();
  EXPECT_EQ(1UL, count);
}

TEST_F(ConfirmationsCreateConfirmationRequestTest, GetAcceptHeaderValue) {
  // Arrange

  // Act
  auto accept_header_value = request_->GetAcceptHeaderValue();

  // Assert
  EXPECT_EQ(accept_header_value, "application/json");
}

TEST_F(ConfirmationsCreateConfirmationRequestTest, GetContentType) {
  // Arrange

  // Act
  auto content_type = request_->GetContentType();

  // Assert
  EXPECT_EQ(content_type, "application/json");
}

TEST_F(
    ConfirmationsCreateConfirmationRequestTest,
    CreateConfirmationRequestDTO_Viewed) {
  // Arrange
  std::string creative_instance_id = "546fe7b0-5047-4f28-a11c-81f14edcf0f6";

  std::string blinded_token_base64 =
      "PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=";
  auto blinded_token = BlindedToken::decode_base64(blinded_token_base64);

  // Act
  auto payload = request_->CreateConfirmationRequestDTO(creative_instance_id,
      blinded_token, ConfirmationType::VIEW);

  // Assert
  std::string expected_payload = R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"type":"view"})";  // NOLINT
  EXPECT_EQ(expected_payload, payload);
}

TEST_F(ConfirmationsCreateConfirmationRequestTest, CreateCredential_Viewed) {
  // Arrange
  std::string unblinded_token_base64 = "PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuYL5mwA8CU2aFMlJrt3DDgC3B1+VD/uyHPfa/+bwYRrpVH5YwNSDEydVx8S4r+BYVY";  // NOLINT

  TokenInfo token_info;
  token_info.unblinded_token =
      UnblindedToken::decode_base64(unblinded_token_base64);
  token_info.public_key = "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=";

  std::string creative_instance_id = "546fe7b0-5047-4f28-a11c-81f14edcf0f6";

  std::string blinded_token_base64 =
      "PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=";
  auto blinded_token = BlindedToken::decode_base64(blinded_token_base64);

  auto payload = request_->CreateConfirmationRequestDTO(creative_instance_id,
      blinded_token, ConfirmationType::VIEW);

  // Act
  auto credential = request_->CreateCredential(token_info, payload);

  // Assert
  std::string expected_credential = "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=";  // NOLINT
  EXPECT_EQ(expected_credential, credential);
}

}  // namespace confirmations
