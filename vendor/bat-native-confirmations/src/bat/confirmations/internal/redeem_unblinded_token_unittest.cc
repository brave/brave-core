/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "net/http/http_status_code.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/confirmations/internal/confirmations_client_mock.h"
#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/create_confirmation_request.h"
#include "bat/confirmations/internal/platform_helper_mock.h"
#include "bat/confirmations/internal/redeem_unblinded_token.h"
#include "bat/confirmations/internal/redeem_unblinded_token_delegate_mock.h"
#include "bat/confirmations/internal/unblinded_tokens.h"
#include "bat/confirmations/internal/unittest_utils.h"

// npm run test -- brave_unit_tests --filter=BatConfirmations*

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

namespace confirmations {

class BatConfirmationsRedeemUnblindedTokenTest : public ::testing::Test {
 protected:
  BatConfirmationsRedeemUnblindedTokenTest()
      : confirmations_client_mock_(std::make_unique<
            NiceMock<ConfirmationsClientMock>>()),
        confirmations_(std::make_unique<
            NiceMock<ConfirmationsImpl>>(confirmations_client_mock_.get())),
        platform_helper_mock_(std::make_unique<
            NiceMock<PlatformHelperMock>>()),
        unblinded_tokens_(std::make_unique<
            UnblindedTokens>(confirmations_.get())),
        unblinded_payment_tokens_(std::make_unique<
            UnblindedTokens>(confirmations_.get())),
        redeem_token_delegate_mock_(std::make_unique<
            NiceMock<RedeemUnblindedTokenDelegateMock>>()),
        redeem_unblinded_token_(std::make_unique<
            RedeemUnblindedToken>(confirmations_.get(), unblinded_tokens_.get(),
                unblinded_payment_tokens_.get())) {
    // You can do set-up work for each test here

    PlatformHelper::GetInstance()->set_for_testing(platform_helper_mock_.get());

    redeem_unblinded_token_->set_delegate(redeem_token_delegate_mock_.get());
  }

  ~BatConfirmationsRedeemUnblindedTokenTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    ON_CALL(*platform_helper_mock_, GetPlatformName())
        .WillByDefault(Return("test"));

    MockLoadState(confirmations_client_mock_);
    MockSaveState(confirmations_client_mock_);

    MockClientInfo(confirmations_client_mock_, "test");

    Initialize(confirmations_);
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  void SetUnblindedTokens() {
    TokenInfo token;
    const std::string unblinded_token = R"(VWKEdIb8nMwmT1eLtNLGufVe6NQBE/SXjBpylLYTVMJTT+fNHI2VBd2ztYqIpEWleazN+0bNc4avKfkcv2FL7oDtt5pyGLYEdainxd+EYcFCxzFt/8638aBxsyFcd+pY)";
    token.unblinded_token = UnblindedToken::decode_base64(unblinded_token);
    token.public_key = "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=";

    unblinded_tokens_->SetTokens({token});
  }

  ConfirmationInfo GetConfirmationInfo() {
    ConfirmationInfo confirmation;
    confirmation.id = "9fd71bc4-1b8e-4c1e-8ddc-443193a09f91";

    confirmation.creative_instance_id = "70829d71-ce2e-4483-a4c0-e1e2bee96520";

    confirmation.type = ConfirmationType::kViewed;

    const TokenInfo token = unblinded_tokens_->GetToken();
    unblinded_tokens_->RemoveToken(token);
    confirmation.token_info = token;

    const std::string payment_token_base64 = R"(aXZNwft34oG2JAVBnpYh/ktTOzr2gi0lKosYNczUUz6ZS9gaDTJmU2FHFps9dIq+QoDwjSjctR5v0rRn+dYo+AHScVqFAgJ5t2s4KtSyawW10gk6hfWPQw16Q0+8u5AG)";
    confirmation.payment_token = Token::decode_base64(payment_token_base64);

    const std::string blinded_payment_token_base64 = R"(Ev5JE4/9TZI/5TqyN9JWfJ1To0HBwQw2rWeAPcdjX3Q=)";
    confirmation.blinded_payment_token =
        BlindedToken::decode_base64(blinded_payment_token_base64);

    const std::string platform =
        PlatformHelper::GetInstance()->GetPlatformName();
    const CreateConfirmationRequest request(confirmations_.get());
    const std::string payload = request.CreateConfirmationRequestDTO(
        confirmation, "test", platform, "US");
    confirmation.credential = request.CreateCredential(token, payload);
    confirmation.timestamp_in_seconds = 1587127747;

    confirmation.created = false;

    return confirmation;
  }

  base::test::TaskEnvironment task_environment_;

  std::unique_ptr<ConfirmationsClientMock> confirmations_client_mock_;
  std::unique_ptr<ConfirmationsImpl> confirmations_;

  std::unique_ptr<PlatformHelperMock> platform_helper_mock_;

  std::unique_ptr<UnblindedTokens> unblinded_tokens_;
  std::unique_ptr<UnblindedTokens> unblinded_payment_tokens_;

  std::unique_ptr<RedeemUnblindedTokenDelegateMock> redeem_token_delegate_mock_;

  std::unique_ptr<RedeemUnblindedToken> redeem_unblinded_token_;
};

TEST_F(BatConfirmationsRedeemUnblindedTokenTest,
    RedeemUnblindedToken) {
  // Arrange
  ON_CALL(*confirmations_client_mock_, LoadURL(_, _, _, _, _, _))
      .WillByDefault(Invoke([](
          const std::string& url,
          const std::vector<std::string>& headers,
          const std::string& content,
          const std::string& content_type,
          const URLRequestMethod method,
          URLRequestCallback callback) {
        int response_status_code = -1;
        std::string response_body;

        const std::string create_confirmation_endpoint = R"(/v1/confirmation/9fd71bc4-1b8e-4c1e-8ddc-443193a09f91/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVRxeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidGVzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjMC1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIsXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVURyR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYmJmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TWGpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3Zz09In0=)";
        const std::string fetch_payment_token_endpoint = R"(/v1/confirmation/9fd71bc4-1b8e-4c1e-8ddc-443193a09f91/paymentToken)";

        const std::string endpoint = GetPathForRequest(url);
        if (endpoint == create_confirmation_endpoint) {
          response_status_code = net::HTTP_CREATED;
          response_body = R"(
            {
              "id" : "9fd71bc4-1b8e-4c1e-8ddc-443193a09f91",
              "payload" : {},
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "70829d71-ce2e-4483-a4c0-e1e2bee96520"
            }
          )";
        } else if (endpoint == fetch_payment_token_endpoint) {
          response_status_code = net::HTTP_OK;
          response_body = R"(
            {
              "id" : "9fd71bc4-1b8e-4c1e-8ddc-443193a09f91",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.736Z",
              "creativeInstanceId" : "70829d71-ce2e-4483-a4c0-e1e2bee96520",
              "paymentToken" : {
                "publicKey" : "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=",
                "batchProof" : "FWTZ5fOYITYlMWMYaxg254QWs+Pmd0dHzoor0mzIlQ8tWHagc7jm7UVJykqIo+ZSM+iK29mPuWJxPHpG4HypBw==",
                "signedTokens" : [
                  "DHe4S37Cn1WaTbCC+ytiNTB2s5H0vcLzVcRgzRoO3lU="
                ]
              }
            }
          )";
        }

        UrlResponse response;
        response.body = response_body;
        response.status_code = response_status_code;
        callback(response);
      }));

  EXPECT_CALL(*confirmations_client_mock_, LoadURL(_, _, _, _, _, _))
      .Times(2);

  SetUnblindedTokens();

  const ConfirmationInfo confirmation = GetConfirmationInfo();

  // Act
  ConfirmationInfo expected_confirmation = confirmation;
  expected_confirmation.created = true;

  EXPECT_CALL(*redeem_token_delegate_mock_,
      OnDidRedeemUnblindedToken(expected_confirmation)).Times(1);

  EXPECT_CALL(*redeem_token_delegate_mock_,
      OnFailedToRedeemUnblindedToken(expected_confirmation)).Times(0);

  redeem_unblinded_token_->Redeem(confirmation);

  // Assert
}

TEST_F(BatConfirmationsRedeemUnblindedTokenTest,
    RetryRedeemingUnblindedToken) {
  // Arrange
  ON_CALL(*confirmations_client_mock_, LoadURL(_, _, _, _, _, _))
      .WillByDefault(Invoke([](
          const std::string& url,
          const std::vector<std::string>& headers,
          const std::string& content,
          const std::string& content_type,
          const URLRequestMethod method,
          URLRequestCallback callback) {
        int response_status_code = -1;
        std::string response_body;

        const std::string fetch_payment_token_endpoint = R"(/v1/confirmation/9fd71bc4-1b8e-4c1e-8ddc-443193a09f91/paymentToken)";

        const std::string endpoint = GetPathForRequest(url);
        if (endpoint == fetch_payment_token_endpoint) {
          response_status_code = net::HTTP_OK;
          response_body = R"(
            {
              "id" : "9fd71bc4-1b8e-4c1e-8ddc-443193a09f91",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.736Z",
              "creativeInstanceId" : "70829d71-ce2e-4483-a4c0-e1e2bee96520",
              "paymentToken" : {
                "publicKey" : "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=",
                "batchProof" : "FWTZ5fOYITYlMWMYaxg254QWs+Pmd0dHzoor0mzIlQ8tWHagc7jm7UVJykqIo+ZSM+iK29mPuWJxPHpG4HypBw==",
                "signedTokens" : [
                  "DHe4S37Cn1WaTbCC+ytiNTB2s5H0vcLzVcRgzRoO3lU="
                ]
              }
            }
          )";
        }

        UrlResponse response;
        response.body = response_body;
        response.status_code = response_status_code;
        callback(response);
      }));

  EXPECT_CALL(*confirmations_client_mock_, LoadURL(_, _, _, _, _, _))
      .Times(1);

  SetUnblindedTokens();

  ConfirmationInfo confirmation = GetConfirmationInfo();
  confirmation.created = true;

  // Act
  ConfirmationInfo expected_confirmation = confirmation;

  EXPECT_CALL(*redeem_token_delegate_mock_,
      OnDidRedeemUnblindedToken(expected_confirmation)).Times(1);

  EXPECT_CALL(*redeem_token_delegate_mock_,
      OnFailedToRedeemUnblindedToken(expected_confirmation)).Times(0);

  redeem_unblinded_token_->Redeem(confirmation);

  // Assert
}

TEST_F(BatConfirmationsRedeemUnblindedTokenTest,
    FailedToRedeemUnblindedTokenDueToFetchPaymentTokenRespondingWith404NotFound) {  // NOLINT
  // Arrange
  ON_CALL(*confirmations_client_mock_, LoadURL(_, _, _, _, _, _))
      .WillByDefault(Invoke([](
          const std::string& url,
          const std::vector<std::string>& headers,
          const std::string& content,
          const std::string& content_type,
          const URLRequestMethod method,
          URLRequestCallback callback) {
        int response_status_code = -1;

        const std::string create_confirmation_endpoint = R"(/v1/confirmation/9fd71bc4-1b8e-4c1e-8ddc-443193a09f91/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVRxeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidGVzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjMC1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIsXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVURyR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYmJmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TWGpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3Zz09In0=)";
        const std::string fetch_payment_token_endpoint = R"(/v1/confirmation/9fd71bc4-1b8e-4c1e-8ddc-443193a09f91/paymentToken)";

        const std::string endpoint = GetPathForRequest(url);
        if (endpoint == create_confirmation_endpoint) {
          response_status_code = net::HTTP_BAD_REQUEST;
        } else if (endpoint == fetch_payment_token_endpoint) {
          response_status_code = net::HTTP_NOT_FOUND;
        }

        UrlResponse response;
        response.status_code = response_status_code;
        callback(response);
      }));

  EXPECT_CALL(*confirmations_client_mock_, LoadURL(_, _, _, _, _, _))
      .Times(2);

  SetUnblindedTokens();

  const ConfirmationInfo confirmation = GetConfirmationInfo();

  // Act
  ConfirmationInfo expected_confirmation = confirmation;
  expected_confirmation.created = false;  // Should retry with new confirmation

  EXPECT_CALL(*redeem_token_delegate_mock_,
      OnDidRedeemUnblindedToken(expected_confirmation)).Times(0);

  EXPECT_CALL(*redeem_token_delegate_mock_,
      OnFailedToRedeemUnblindedToken(expected_confirmation)).Times(1);

  redeem_unblinded_token_->Redeem(confirmation);

  // Assert
}

TEST_F(BatConfirmationsRedeemUnblindedTokenTest,
    FailedToRedeemUnblindedTokenDueToFetchPaymentTokenRespondingWith500InternalServerError) {  // NOLINT
  // Arrange
  ON_CALL(*confirmations_client_mock_, LoadURL(_, _, _, _, _, _))
      .WillByDefault(Invoke([](
          const std::string& url,
          const std::vector<std::string>& headers,
          const std::string& content,
          const std::string& content_type,
          const URLRequestMethod method,
          URLRequestCallback callback) {
        int response_status_code = -1;

        const std::string create_confirmation_endpoint = R"(/v1/confirmation/9fd71bc4-1b8e-4c1e-8ddc-443193a09f91/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVRxeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidGVzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjMC1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIsXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVURyR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYmJmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TWGpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3Zz09In0=)";
        const std::string fetch_payment_token_endpoint = R"(/v1/confirmation/9fd71bc4-1b8e-4c1e-8ddc-443193a09f91/paymentToken)";

        const std::string endpoint = GetPathForRequest(url);
        if (endpoint == create_confirmation_endpoint) {
          response_status_code = net::HTTP_OK;
        } else if (endpoint == fetch_payment_token_endpoint) {
          response_status_code = net::HTTP_INTERNAL_SERVER_ERROR;
        }

        UrlResponse response;
        response.status_code = response_status_code;
        callback(response);
      }));

  EXPECT_CALL(*confirmations_client_mock_, LoadURL(_, _, _, _, _, _))
      .Times(2);

  SetUnblindedTokens();

  const ConfirmationInfo confirmation = GetConfirmationInfo();

  // Act
  ConfirmationInfo expected_confirmation = confirmation;
  expected_confirmation.created = true;  // Should retry with same confirmation

  EXPECT_CALL(*redeem_token_delegate_mock_,
      OnDidRedeemUnblindedToken(expected_confirmation)).Times(0);

  EXPECT_CALL(*redeem_token_delegate_mock_,
      OnFailedToRedeemUnblindedToken(expected_confirmation)).Times(1);

  redeem_unblinded_token_->Redeem(confirmation);

  // Assert
}

}  // namespace confirmations
