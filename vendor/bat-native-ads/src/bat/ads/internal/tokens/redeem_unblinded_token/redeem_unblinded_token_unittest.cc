/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_token/redeem_unblinded_token.h"

#include <memory>
#include <string>

#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/create_confirmation_url_request_builder.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/create_confirmation_util.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/redeem_unblinded_token_delegate_mock.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::_;
using ::testing::NiceMock;

namespace ads {

using challenge_bypass_ristretto::PublicKey;

class BatAdsRedeemUnblindedTokenTest : public UnitTestBase {
 protected:
  BatAdsRedeemUnblindedTokenTest()
      : redeem_unblinded_token_(std::make_unique<RedeemUnblindedToken>()),
        redeem_unblinded_token_delegate_mock_(
            std::make_unique<NiceMock<RedeemUnblindedTokenDelegateMock>>()) {
    redeem_unblinded_token_->set_delegate(
        redeem_unblinded_token_delegate_mock_.get());
  }

  ~BatAdsRedeemUnblindedTokenTest() override = default;

  privacy::UnblindedTokens* get_unblinded_tokens() {
    return ConfirmationsState::Get()->get_unblinded_tokens();
  }

  void SetUnblindedTokens() {
    privacy::UnblindedTokenInfo unblinded_token;

    const std::string unblinded_token_base64 =
        R"(VWKEdIb8nMwmT1eLtNLGufVe6NQBE/SXjBpylLYTVMJTT+fNHI2VBd2ztYqIpEWleazN+0bNc4avKfkcv2FL7oDtt5pyGLYEdainxd+EYcFCxzFt/8638aBxsyFcd+pY)";
    unblinded_token.value =
        privacy::UnblindedToken::decode_base64(unblinded_token_base64);

    unblinded_token.public_key = PublicKey::decode_base64(
        "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=");

    get_unblinded_tokens()->SetTokens({unblinded_token});
  }

  ConfirmationInfo GetConfirmationInfo() {
    ConfirmationInfo confirmation;
    confirmation.id = "9fd71bc4-1b8e-4c1e-8ddc-443193a09f91";

    confirmation.creative_instance_id = "70829d71-ce2e-4483-a4c0-e1e2bee96520";

    confirmation.type = ConfirmationType::kViewed;

    if (!get_unblinded_tokens()->IsEmpty()) {
      const privacy::UnblindedTokenInfo unblinded_token =
          get_unblinded_tokens()->GetToken();
      get_unblinded_tokens()->RemoveToken(unblinded_token);
      confirmation.unblinded_token = unblinded_token;

      const std::string payment_token_base64 =
          R"(aXZNwft34oG2JAVBnpYh/ktTOzr2gi0lKosYNczUUz6ZS9gaDTJmU2FHFps9dIq+QoDwjSjctR5v0rRn+dYo+AHScVqFAgJ5t2s4KtSyawW10gk6hfWPQw16Q0+8u5AG)";
      confirmation.payment_token = Token::decode_base64(payment_token_base64);

      const std::string blinded_payment_token_base64 =
          R"(Ev5JE4/9TZI/5TqyN9JWfJ1To0HBwQw2rWeAPcdjX3Q=)";
      confirmation.blinded_payment_token =
          BlindedToken::decode_base64(blinded_payment_token_base64);

      const std::string payload = CreateConfirmationRequestDTO(confirmation);
      confirmation.credential = CreateCredential(unblinded_token, payload);
    }

    confirmation.created_at = TimestampToTime(1587127747);

    confirmation.was_created = false;

    return confirmation;
  }

  std::unique_ptr<RedeemUnblindedToken> redeem_unblinded_token_;
  std::unique_ptr<RedeemUnblindedTokenDelegateMock>
      redeem_unblinded_token_delegate_mock_;
};

TEST_F(BatAdsRedeemUnblindedTokenTest, RedeemUnblindedToken) {
  // Arrange
  const URLEndpoints endpoints = {
      {// Create confirmation request
       R"(/v1/confirmation/9fd71bc4-1b8e-4c1e-8ddc-443193a09f91/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVRxeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidGVzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjMC1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIsXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVURyR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYmJmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TWGpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3Zz09In0=)",
       {{net::HTTP_CREATED, R"(
            {
              "id" : "9fd71bc4-1b8e-4c1e-8ddc-443193a09f91",
              "payload" : {},
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "70829d71-ce2e-4483-a4c0-e1e2bee96520"
            }
          )"}}},
      {// Fetch payment token request
       R"(/v1/confirmation/9fd71bc4-1b8e-4c1e-8ddc-443193a09f91/paymentToken)",
       {{net::HTTP_OK, R"(
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
          )"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  SetUnblindedTokens();

  const ConfirmationInfo confirmation = GetConfirmationInfo();

  // Act
  ConfirmationInfo expected_confirmation = confirmation;
  expected_confirmation.was_created = true;

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnDidRedeemUnblindedToken(expected_confirmation, _))
      .Times(1);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToRedeemUnblindedToken(_, _))
      .Times(0);

  redeem_unblinded_token_->Redeem(confirmation);

  // Assert
}

TEST_F(BatAdsRedeemUnblindedTokenTest, RetryRedeemingUnblindedToken) {
  // Arrange
  const URLEndpoints endpoints = {
      {// Fetch payment token request
       R"(/v1/confirmation/9fd71bc4-1b8e-4c1e-8ddc-443193a09f91/paymentToken)",
       {{net::HTTP_OK, R"(
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
          )"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  SetUnblindedTokens();

  ConfirmationInfo confirmation = GetConfirmationInfo();
  confirmation.was_created = true;

  // Act
  ConfirmationInfo expected_confirmation = confirmation;

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnDidRedeemUnblindedToken(expected_confirmation, _))
      .Times(1);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToRedeemUnblindedToken(_, _))
      .Times(0);

  redeem_unblinded_token_->Redeem(confirmation);

  // Assert
}

TEST_F(
    BatAdsRedeemUnblindedTokenTest,
    FailedToRedeemUnblindedTokenDueToFetchPaymentTokenRespondingWith404NotFound) {  // NOLINT
  // Arrange
  const URLEndpoints endpoints = {
      {// Create confirmation request
       R"(/v1/confirmation/9fd71bc4-1b8e-4c1e-8ddc-443193a09f91/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVRxeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidGVzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjMC1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIsXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVURyR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYmJmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TWGpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3Zz09In0=)",
       {{net::HTTP_BAD_REQUEST, ""}}},
      {// Fetch payment token request
       R"(/v1/confirmation/9fd71bc4-1b8e-4c1e-8ddc-443193a09f91/paymentToken)",
       {{net::HTTP_NOT_FOUND, ""}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  SetUnblindedTokens();

  const ConfirmationInfo confirmation = GetConfirmationInfo();

  // Act
  ConfirmationInfo expected_confirmation = confirmation;
  expected_confirmation.was_created = false;

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnDidRedeemUnblindedToken(_, _))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToRedeemUnblindedToken(expected_confirmation, true))
      .Times(1);

  redeem_unblinded_token_->Redeem(confirmation);

  // Assert
}

TEST_F(
    BatAdsRedeemUnblindedTokenTest,
    FailedToRedeemUnblindedTokenDueToFetchPaymentTokenRespondingWith500InternalServerError) {  // NOLINT
  // Arrange
  const URLEndpoints endpoints = {
      {// Create confirmation request
       R"(/v1/confirmation/9fd71bc4-1b8e-4c1e-8ddc-443193a09f91/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVRxeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidGVzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjMC1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIsXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVURyR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYmJmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TWGpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3Zz09In0=)",
       {{net::HTTP_OK, ""}}},
      {// Fetch payment token request
       R"(/v1/confirmation/9fd71bc4-1b8e-4c1e-8ddc-443193a09f91/paymentToken)",
       {{net::HTTP_INTERNAL_SERVER_ERROR, ""}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  SetUnblindedTokens();

  const ConfirmationInfo confirmation = GetConfirmationInfo();

  // Act
  ConfirmationInfo expected_confirmation = confirmation;
  expected_confirmation.was_created = true;

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnDidRedeemUnblindedToken(_, _))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToRedeemUnblindedToken(expected_confirmation, true))
      .Times(1);

  redeem_unblinded_token_->Redeem(confirmation);

  // Assert
}

TEST_F(BatAdsRedeemUnblindedTokenTest, RedeemUnblindedTokenIfAdsIsDisabled) {
  // Arrange
  const URLEndpoints endpoints = {
      {// Create confirmation request
       "/v1/confirmation/9fd71bc4-1b8e-4c1e-8ddc-443193a09f91",
       {{418 /* I'm a teapot */, R"(
            {
              "id" : "9fd71bc4-1b8e-4c1e-8ddc-443193a09f91",
              "payload" : {},
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "70829d71-ce2e-4483-a4c0-e1e2bee96520"
            }
          )"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  const ConfirmationInfo confirmation = GetConfirmationInfo();

  // Act
  ConfirmationInfo expected_confirmation = confirmation;

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnDidSendConfirmation(expected_confirmation))
      .Times(1);

  redeem_unblinded_token_->Redeem(confirmation);

  // Assert
}

}  // namespace ads
