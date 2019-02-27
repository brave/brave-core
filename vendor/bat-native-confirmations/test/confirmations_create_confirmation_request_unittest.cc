/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <memory>

#include "confirmations_client_mock.h"
#include "bat-native-confirmations/src/confirmations_impl.h"
#include "bat-native-confirmations/src/create_confirmation_request.h"
#include "bat-native-confirmations/src/security_helper.h"
#include "bat-native-confirmations/include/bat/confirmations/wallet_info.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

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
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case
};

TEST_F(ConfirmationsCreateConfirmationRequestTest, BuildUrl) {
  // Arrange
  std::string confirmation_id = "c7f8c42d-6768-4dd7-8dc6-612cbba3ec21";
  std::string credential = "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiQUZwNzMyaStXUU5lMUtGb0NJVFpMWkVDZmtYM0pHY3Fvc2lKbSt5KzRGTT1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTg0MWE0NmUtNjBmMi00ZTAxLWFhMDAtYmEyMzZiZDEyY2NhXCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoienFyYzV1TlF5Mm12QS9RQ01XSDFyOHg3dEdDL1pBTFZJdno2M1ZXd3lRQy8zaDZTVWI2OXhEQmdPYTA3NmFJcUpGNDA3dVZ1TGM2bTFsSzFpUGxkM3c9PSIsInQiOiJPcnJjMlFkS0VRaERLRVk3NmNGRThqOVRreUIrbVBJV0h5TzhVcFErOGQraW5UblZnaWdidlZYYTd1TFJRWnRLdml1a2pQamdiYjk2THpwTVEzQzJHUT09In0=";  // NOLINT

  // Act
  auto url = request_->BuildUrl(confirmation_id, credential);

  // Assert
  std::string expected_url = "https://ads-serve.bravesoftware.com/v1/confirmation/c7f8c42d-6768-4dd7-8dc6-612cbba3ec21/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiQUZwNzMyaStXUU5lMUtGb0NJVFpMWkVDZmtYM0pHY3Fvc2lKbSt5KzRGTT1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTg0MWE0NmUtNjBmMi00ZTAxLWFhMDAtYmEyMzZiZDEyY2NhXCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoienFyYzV1TlF5Mm12QS9RQ01XSDFyOHg3dEdDL1pBTFZJdno2M1ZXd3lRQy8zaDZTVWI2OXhEQmdPYTA3NmFJcUpGNDA3dVZ1TGM2bTFsSzFpUGxkM3c9PSIsInQiOiJPcnJjMlFkS0VRaERLRVk3NmNGRThqOVRreUIrbVBJV0h5TzhVcFErOGQraW5UblZnaWdidlZYYTd1TFJRWnRLdml1a2pQamdiYjk2THpwTVEzQzJHUT09In0=";  // NOLINT
  EXPECT_EQ(expected_url, url);
}

TEST_F(ConfirmationsCreateConfirmationRequestTest, GetMethod) {
  // Arrange

  // Act
  auto method = request_->GetMethod();

  // Assert
  EXPECT_EQ(URLRequestMethod::POST, method);
}

TEST_F(ConfirmationsCreateConfirmationRequestTest, BuildBody) {
  // Arrange
  std::string creative_instance_id = "465e08ad-03be-42ee-902a-dc88688aa2cb";

  std::string blinded_token_base64 =
      "FvnSTMJ6dSeinPIdc3P2XQlv84Y1wcljzWmkfinVXHs=";
  auto blinded_token = BlindedToken::decode_base64(blinded_token_base64);

  auto payload = request_->CreateConfirmationRequestDTO(creative_instance_id,
      blinded_token);

  // Act
  auto body = request_->BuildBody(payload);

  // Assert
  std::string expected_body = R"({"blindedPaymentToken":"FvnSTMJ6dSeinPIdc3P2XQlv84Y1wcljzWmkfinVXHs=","creativeInstanceId":"465e08ad-03be-42ee-902a-dc88688aa2cb","payload":{},"type":"view"})";  // NOLINT
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

TEST_F(ConfirmationsCreateConfirmationRequestTest,
    CreateConfirmationRequestDTO) {
  // Arrange
  std::string creative_instance_id = "465e08ad-03be-42ee-902a-dc88688aa2cb";

  std::string blinded_token_base64 =
      "FvnSTMJ6dSeinPIdc3P2XQlv84Y1wcljzWmkfinVXHs=";
  auto blinded_token = BlindedToken::decode_base64(blinded_token_base64);

  // Act
  auto payload = request_->CreateConfirmationRequestDTO(creative_instance_id,
      blinded_token);

  // Assert
  std::string expected_payload = R"({"blindedPaymentToken":"FvnSTMJ6dSeinPIdc3P2XQlv84Y1wcljzWmkfinVXHs=","creativeInstanceId":"465e08ad-03be-42ee-902a-dc88688aa2cb","payload":{},"type":"view"})";  // NOLINT
  EXPECT_EQ(expected_payload, payload);
}

TEST_F(ConfirmationsCreateConfirmationRequestTest, CreateCredential) {
  // Arrange
  std::string unblinded_token_base64 = "Orrc2QdKEQhDKEY76cFE8j9TkyB+mPIWHyO8UpQ+8d+inTnVgigbvVXa7uLRQZtKviukjPjgbb96LzpMQ3C2GY7X7c2oL0nZiXeiGEsgkKYJWWDveLNCnT3zxpWJbFkR";  // NOLINT
  auto unblinded_token = UnblindedToken::decode_base64(unblinded_token_base64);

  std::string creative_instance_id = "5841a46e-60f2-4e01-aa00-ba236bd12cca";

  std::string blinded_token_base64 =
      "AFp732i+WQNe1KFoCITZLZECfkX3JGcqosiJm+y+4FM=";
  auto blinded_token = BlindedToken::decode_base64(blinded_token_base64);

  auto payload = request_->CreateConfirmationRequestDTO(creative_instance_id,
      blinded_token);

  // Act
  auto credential = request_->CreateCredential(unblinded_token, payload);

  // Assert
  std::string expected_credential = "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiQUZwNzMyaStXUU5lMUtGb0NJVFpMWkVDZmtYM0pHY3Fvc2lKbSt5KzRGTT1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTg0MWE0NmUtNjBmMi00ZTAxLWFhMDAtYmEyMzZiZDEyY2NhXCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoienFyYzV1TlF5Mm12QS9RQ01XSDFyOHg3dEdDL1pBTFZJdno2M1ZXd3lRQy8zaDZTVWI2OXhEQmdPYTA3NmFJcUpGNDA3dVZ1TGM2bTFsSzFpUGxkM3c9PSIsInQiOiJPcnJjMlFkS0VRaERLRVk3NmNGRThqOVRreUIrbVBJV0h5TzhVcFErOGQraW5UblZnaWdidlZYYTd1TFJRWnRLdml1a2pQamdiYjk2THpwTVEzQzJHUT09In0=";  // NOLINT
  EXPECT_EQ(expected_credential, credential);
}

}  // namespace confirmations
