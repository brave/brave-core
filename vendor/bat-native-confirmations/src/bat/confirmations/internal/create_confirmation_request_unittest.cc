/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/create_confirmation_request.h"

#include <memory>
#include <string>
#include <vector>

#include "base/strings/stringprintf.h"
#include "base/test/task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/confirmations/internal/confirmations_client_mock.h"
#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/platform_helper.h"
#include "bat/confirmations/internal/unittest_utils.h"

// npm run test -- brave_unit_tests --filter=BatConfirmations*

using ::testing::NiceMock;

namespace confirmations {

class BatConfirmationsCreateConfirmationRequestTest : public ::testing::Test {
 protected:
  BatConfirmationsCreateConfirmationRequestTest()
      : confirmations_client_mock_(std::make_unique<
            NiceMock<ConfirmationsClientMock>>()),
        confirmations_(std::make_unique<ConfirmationsImpl>(
            confirmations_client_mock_.get())),
        request_(std::make_unique<CreateConfirmationRequest>(
            confirmations_.get())) {
    // You can do set-up work for each test here
  }

  ~BatConfirmationsCreateConfirmationRequestTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    MockLoadState(confirmations_client_mock_);
    MockSaveState(confirmations_client_mock_);

    Initialize(confirmations_);
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  ConfirmationInfo CreateConfirmationInfo() {
    ConfirmationInfo confirmation;

    confirmation.creative_instance_id = "546fe7b0-5047-4f28-a11c-81f14edcf0f6";

    confirmation.type = ConfirmationType::kViewed;

    const std::string blinded_token_base64 =
        "PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=";
    confirmation.blinded_payment_token =
        BlindedToken::decode_base64(blinded_token_base64);

    return confirmation;
  }

  base::test::TaskEnvironment task_environment_;

  std::unique_ptr<ConfirmationsClientMock> confirmations_client_mock_;
  std::unique_ptr<ConfirmationsImpl> confirmations_;

  std::unique_ptr<CreateConfirmationRequest> request_;
};

TEST_F(BatConfirmationsCreateConfirmationRequestTest,
    BuildUrl) {
  // Arrange
  const std::string confirmation_id = "d990ed8d-d739-49fb-811b-c2e02158fb60";

  const std::string credential =
      "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdW"
      "Rkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZl"
      "SW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNm"
      "MGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0"
      "dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJY"
      "MnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIs"
      "InQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUy"
      "WEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=";

  // Act
  const std::string url = request_->BuildUrl(confirmation_id, credential);

  // Assert
  const std::string expected_url = R"(https://ads-serve.bravesoftware.com/v1/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdWRkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiNTQ2ZmU3YjAtNTA0Ny00ZjI4LWExMWMtODFmMTRlZGNmMGY2XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoibGRWYWxyb2hqNWFIWW1FdWMvUmpIYTAweFdMdFJWY0hGMS9XWnl4ZGJYMnhkQ1ByMFgyMVg3cWtKVUxRdUw4U2JWWHJUT3lEbTJJNkFrT0R0SHYxR2c9PSIsInQiOiJQTG93ejJXRjJlR0Q1emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFzT3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=)";

  EXPECT_EQ(expected_url, url);
}

TEST_F(BatConfirmationsCreateConfirmationRequestTest,
    GetMethod) {
  // Arrange

  // Act
  const URLRequestMethod method = request_->GetMethod();

  // Assert
  EXPECT_EQ(URLRequestMethod::POST, method);
}

TEST_F(BatConfirmationsCreateConfirmationRequestTest,
    BuildBodyForViewedConfirmation) {
  // Arrange
  const ConfirmationInfo confirmation = CreateConfirmationInfo();

  const std::string platform = PlatformHelper::GetInstance()->GetPlatformName();

  const std::string payload = request_->CreateConfirmationRequestDTO(
      confirmation, "release", platform, "US");

  // Act
  const std::string body = request_->BuildBody(payload);

  // Assert
  const std::string expected_body = base::StringPrintf(R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","buildChannel":"release","countryCode":"US","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"platform":"%s","type":"view"})", platform.c_str());  // NOLINT

  EXPECT_EQ(expected_body, body);
}

TEST_F(BatConfirmationsCreateConfirmationRequestTest,
    HeadersCount) {
  // Arrange

  // Act
  const std::vector<std::string> headers = request_->BuildHeaders();

  // Assert
  const size_t count = headers.size();
  EXPECT_EQ(1UL, count);
}

TEST_F(BatConfirmationsCreateConfirmationRequestTest,
    GetAcceptHeaderValue) {
  // Arrange

  // Act
  const std::string accept_header_value = request_->GetAcceptHeaderValue();

  // Assert
  EXPECT_EQ("application/json", accept_header_value);
}

TEST_F(BatConfirmationsCreateConfirmationRequestTest,
    GetContentType) {
  // Arrange

  // Act
  const std::string content_type = request_->GetContentType();

  // Assert
  EXPECT_EQ("application/json", content_type);
}

TEST_F(BatConfirmationsCreateConfirmationRequestTest,
    CreateConfirmationRequestDTOForViewedConfirmation) {
  // Arrange
  const ConfirmationInfo confirmation = CreateConfirmationInfo();

  const std::string platform = PlatformHelper::GetInstance()->GetPlatformName();

  // Act
  const std::string payload = request_->CreateConfirmationRequestDTO(
      confirmation, "release", platform, "US");

  // Assert
  const std::string expected_payload = base::StringPrintf(R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","buildChannel":"release","countryCode":"US","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"platform":"%s","type":"view"})", platform.c_str());  // NOLINT

  EXPECT_EQ(expected_payload, payload);
}

TEST_F(BatConfirmationsCreateConfirmationRequestTest,
    CreateCredentialForViewedConfirmation) {
  // Arrange
  TokenInfo token;
  const std::string unblinded_token_base64 =
      "PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuY"
      "L5mwA8CU2aFMlJrt3DDgC3B1+VD/uyHPfa/+bwYRrpVH5YwNSDEydVx8S4r+BYVY";
  token.unblinded_token = UnblindedToken::decode_base64(unblinded_token_base64);
  token.public_key = "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=";

  const ConfirmationInfo confirmation = CreateConfirmationInfo();

  const std::string payload = request_->CreateConfirmationRequestDTO(
      confirmation, "release", "platform", "US");

  // Act
  const std::string credential = request_->CreateCredential(token, payload);

  // Assert
  const std::string expected_credential =
      "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUEkzbEZxcEdW"
      "Rkt6NFRINXlFd1hJM1IvUW50bVRwVWdlQmFLK1NUaUJ4OD1cIixcImJ1aWxkQ2hh"
      "bm5lbFwiOlwicmVsZWFzZVwiLFwiY291bnRyeUNvZGVcIjpcIlVTXCIsXCJjcmVh"
      "dGl2ZUluc3RhbmNlSWRcIjpcIjU0NmZlN2IwLTUwNDctNGYyOC1hMTFjLTgxZjE0"
      "ZWRjZjBmNlwiLFwicGF5bG9hZFwiOnt9LFwicGxhdGZvcm1cIjpcInBsYXRmb3Jt"
      "XCIsXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiWE1UbElublhHSlYr"
      "RVp3REVtSFFIQWYzTVJSYXJ1LzdhZ1IrUE5CSWF6K2J2YkJPVWlCWjRvQkdaMlNr"
      "L1hLaWM1dTJwNERqZWN3TTUwZFNGUC8wS0E9PSIsInQiOiJQTG93ejJXRjJlR0Q1"
      "emZ3WmprOXA3NkhYQkxES01xLzNFQVpIZUcvZkUyWEdRNDhqeXRlK1ZlNTBabGFz"
      "T3VZTDVtd0E4Q1UyYUZNbEpydDNERGdDdz09In0=";

  EXPECT_EQ(expected_credential, credential);
}

TEST_F(BatConfirmationsCreateConfirmationRequestTest,
    CreateConfirmationRequestDTOForLargeAnonymityCountry) {
  // Arrange
  const ConfirmationInfo confirmation = CreateConfirmationInfo();

  const std::string platform = PlatformHelper::GetInstance()->GetPlatformName();

  // Act
  const std::string payload = request_->CreateConfirmationRequestDTO(
      confirmation, "release", platform, "US");

  // Assert
  const std::string expected_payload = base::StringPrintf(R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","buildChannel":"release","countryCode":"US","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"platform":"%s","type":"view"})", platform.c_str());  // NOLINT

  EXPECT_EQ(expected_payload, payload);
}

TEST_F(BatConfirmationsCreateConfirmationRequestTest,
    CreateConfirmationRequestDTOForOtherCountryCode) {
  // Arrange
  const ConfirmationInfo confirmation = CreateConfirmationInfo();

  const std::string platform = PlatformHelper::GetInstance()->GetPlatformName();

  // Act
  const std::string payload = request_->CreateConfirmationRequestDTO(
      confirmation, "release", platform, "AS");

  // Assert
  const std::string expected_payload = base::StringPrintf(R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","buildChannel":"release","countryCode":"??","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"platform":"%s","type":"view"})", platform.c_str());  // NOLINT

  EXPECT_EQ(expected_payload, payload);
}

TEST_F(BatConfirmationsCreateConfirmationRequestTest,
    CreateConfirmationRequestDTOForSmallAnonymityCountry) {
  // Arrange
  const ConfirmationInfo confirmation = CreateConfirmationInfo();

  const std::string platform = PlatformHelper::GetInstance()->GetPlatformName();

  // Act
  const std::string payload = request_->CreateConfirmationRequestDTO(
      confirmation, "release", platform, "KY");

  // Assert
  const std::string expected_payload = base::StringPrintf(R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","buildChannel":"release","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"platform":"%s","type":"view"})", platform.c_str());  // NOLINT

  EXPECT_EQ(expected_payload, payload);
}

TEST_F(BatConfirmationsCreateConfirmationRequestTest,
    CreateConfirmationRequestDTOForLargeAnonymityCountryAndNonReleaseChannel) {
  // Arrange
  const ConfirmationInfo confirmation = CreateConfirmationInfo();

  const std::string platform = PlatformHelper::GetInstance()->GetPlatformName();

  // Act
  const std::string payload = request_->CreateConfirmationRequestDTO(
      confirmation, "development", platform, "AS");

  // Assert
  const std::string expected_payload = base::StringPrintf(R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","buildChannel":"development","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"platform":"%s","type":"view"})", platform.c_str());  // NOLINT

  EXPECT_EQ(expected_payload, payload);
}

TEST_F(BatConfirmationsCreateConfirmationRequestTest,
    CreateConfirmationRequestDTOForOtherCountryCodeAndNonReleaseChannel) {
  // Arrange
  const ConfirmationInfo confirmation = CreateConfirmationInfo();

  const std::string platform = PlatformHelper::GetInstance()->GetPlatformName();

  // Act
  const std::string payload = request_->CreateConfirmationRequestDTO(
      confirmation, "development", platform, "AS");

  // Assert
  const std::string expected_payload = base::StringPrintf(R"({"blindedPaymentToken":"PI3lFqpGVFKz4TH5yEwXI3R/QntmTpUgeBaK+STiBx8=","buildChannel":"development","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","payload":{},"platform":"%s","type":"view"})", platform.c_str());  // NOLINT

  EXPECT_EQ(expected_payload, payload);
}

}  // namespace confirmations
