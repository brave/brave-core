/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/redeem_unblinded_token/redeem_unblinded_token.h"

#include <memory>

#include "bat/ads/internal/account/confirmations/confirmation_unittest_util.h"
#include "bat/ads/internal/account/issuers/issuers_unittest_util.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_token/redeem_unblinded_token_delegate_mock.h"
#include "bat/ads/internal/common/net/http/http_status_code.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_mock_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

using ::testing::_;
using ::testing::NiceMock;

class BatAdsRedeemUnblindedTokenTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    redeem_unblinded_token_ = std::make_unique<RedeemUnblindedToken>();
    redeem_unblinded_token_delegate_mock_ =
        std::make_unique<NiceMock<RedeemUnblindedTokenDelegateMock>>();
    redeem_unblinded_token_->SetDelegate(
        redeem_unblinded_token_delegate_mock_.get());
  }

  std::unique_ptr<RedeemUnblindedToken> redeem_unblinded_token_;
  std::unique_ptr<RedeemUnblindedTokenDelegateMock>
      redeem_unblinded_token_delegate_mock_;
};

TEST_F(BatAdsRedeemUnblindedTokenTest, RedeemUnblindedTokenIfAdsAreEnabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_CREATED, R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6"
            }
          )"}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_OK, R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.736Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
              "paymentToken" : {
                "publicKey" : "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=",
                "batchProof" : "FWTZ5fOYITYlMWMYaxg254QWs+Pmd0dHzoor0mzIlQ8tWHagc7jm7UVJykqIo+ZSM+iK29mPuWJxPHpG4HypBw==",
                "signedTokens" : [
                  "DHe4S37Cn1WaTbCC+ytiNTB2s5H0vcLzVcRgzRoO3lU="
                ]
              }
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(1);
  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  ConfirmationInfo expected_confirmation = *confirmation;
  expected_confirmation.was_created = true;

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_, OnDidSendConfirmation(_))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToSendConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnDidRedeemUnblindedToken(expected_confirmation, _));

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToRedeemUnblindedToken(_, _, _))
      .Times(0);

  redeem_unblinded_token_->Redeem(*confirmation);

  // Assert
}

TEST_F(BatAdsRedeemUnblindedTokenTest,
       RetryRedeemingUnblindedTokenIfIssuersAreMissingAndAdsAreEnabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  privacy::SetUnblindedTokens(1);
  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_, OnDidSendConfirmation(_))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToSendConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnDidRedeemUnblindedToken(_, _))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToRedeemUnblindedToken(*confirmation,
                                             /*should_retry*/ true,
                                             /*should_backoff*/ true));

  redeem_unblinded_token_->Redeem(*confirmation);

  // Assert
}

TEST_F(BatAdsRedeemUnblindedTokenTest,
       RedeemUnblindedTokenIfConfirmationWasCreatedAndAdsAreEnabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  const URLResponseMap url_responses = {
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_OK, R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.736Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
              "paymentToken" : {
                "publicKey" : "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=",
                "batchProof" : "FWTZ5fOYITYlMWMYaxg254QWs+Pmd0dHzoor0mzIlQ8tWHagc7jm7UVJykqIo+ZSM+iK29mPuWJxPHpG4HypBw==",
                "signedTokens" : [
                  "DHe4S37Cn1WaTbCC+ytiNTB2s5H0vcLzVcRgzRoO3lU="
                ]
              }
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(1);
  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  ConfirmationInfo expected_confirmation = *confirmation;
  expected_confirmation.was_created = true;

  // Act
  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_, OnDidSendConfirmation(_))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToSendConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnDidRedeemUnblindedToken(expected_confirmation, _));

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToRedeemUnblindedToken(_, _, _))
      .Times(0);

  redeem_unblinded_token_->Redeem(*confirmation);

  // Assert
}

TEST_F(
    BatAdsRedeemUnblindedTokenTest,
    FailAndRetryToRedeemUnblindedTokenDueToFetchPaymentTokenRespondingWith404NotFoundIfAdsAreEnabled) {  // NOLINT
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_OK, {}}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_NOT_FOUND, {}}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(1);
  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_, OnDidSendConfirmation(_))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToSendConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnDidRedeemUnblindedToken(_, _))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToRedeemUnblindedToken(*confirmation,
                                             /*should_retry*/ true,
                                             /*should_backoff*/ false));

  redeem_unblinded_token_->Redeem(*confirmation);

  // Assert
}

TEST_F(
    BatAdsRedeemUnblindedTokenTest,
    FailAndRetryToRedeemUnblindedTokenDueToFetchPaymentTokenRespondingWithAcceptedIfAdsAreEnabled) {  // NOLINT
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_OK, {}}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_ACCEPTED, {}}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(1);
  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  ConfirmationInfo expected_confirmation = *confirmation;
  expected_confirmation.was_created = true;

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_, OnDidSendConfirmation(_))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToSendConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnDidRedeemUnblindedToken(_, _))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToRedeemUnblindedToken(expected_confirmation,
                                             /*should_retry*/ true,
                                             /*should_backoff*/ false));

  redeem_unblinded_token_->Redeem(*confirmation);

  // Assert
}

TEST_F(
    BatAdsRedeemUnblindedTokenTest,
    FailAndRetryToRedeemUnblindedTokenDueToFetchPaymentTokenRespondingWith500InternalServerErrorIfAdsAreEnabled) {  // NOLINT
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_OK, {}}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_INTERNAL_SERVER_ERROR, {}}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(1);
  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  ConfirmationInfo expected_confirmation = *confirmation;
  expected_confirmation.was_created = true;

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_, OnDidSendConfirmation(_))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToSendConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnDidRedeemUnblindedToken(_, _))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToRedeemUnblindedToken(expected_confirmation,
                                             /*should_retry*/ true,
                                             /*should_backoff*/ true));

  redeem_unblinded_token_->Redeem(*confirmation);

  // Assert
}

TEST_F(BatAdsRedeemUnblindedTokenTest, SendConfirmationIfAdsIsDisabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a",
       {{net::kHttpImATeapot, R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6"
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnDidSendConfirmation(*confirmation));

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToSendConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnDidRedeemUnblindedToken(_, _))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToRedeemUnblindedToken(_, _, _))
      .Times(0);

  redeem_unblinded_token_->Redeem(*confirmation);

  // Assert
}

TEST_F(BatAdsRedeemUnblindedTokenTest,
       DoNotRetrySendingConfirmationForHttpBadRequestResponseIfAdsIsDisabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a",
       {{net::HTTP_BAD_REQUEST, {}}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_, OnDidSendConfirmation(_))
      .Times(0);

  EXPECT_CALL(
      *redeem_unblinded_token_delegate_mock_,
      OnFailedToSendConfirmation(*confirmation, /*should_retry*/ false));

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnDidRedeemUnblindedToken(_, _))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToRedeemUnblindedToken(_, _, _))
      .Times(0);

  redeem_unblinded_token_->Redeem(*confirmation);

  // Assert
}

TEST_F(BatAdsRedeemUnblindedTokenTest,
       DoNotRetrySendingConfirmationForHttpConflictResponseIfAdsIsDisabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a",
       {{net::HTTP_CONFLICT, {}}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_, OnDidSendConfirmation(_))
      .Times(0);

  EXPECT_CALL(
      *redeem_unblinded_token_delegate_mock_,
      OnFailedToSendConfirmation(*confirmation, /*should_retry*/ false));

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnDidRedeemUnblindedToken(_, _))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToRedeemUnblindedToken(_, _, _))
      .Times(0);

  redeem_unblinded_token_->Redeem(*confirmation);

  // Assert
}

TEST_F(BatAdsRedeemUnblindedTokenTest,
       DoNotRetrySendingConfirmationForHttpCreatedResponseIfAdsIsDisabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a",
       {{net::HTTP_CREATED, {}}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_, OnDidSendConfirmation(_))
      .Times(0);

  EXPECT_CALL(
      *redeem_unblinded_token_delegate_mock_,
      OnFailedToSendConfirmation(*confirmation, /*should_retry*/ false));

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnDidRedeemUnblindedToken(_, _))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToRedeemUnblindedToken(_, _, _))
      .Times(0);

  redeem_unblinded_token_->Redeem(*confirmation);

  // Assert
}

TEST_F(BatAdsRedeemUnblindedTokenTest,
       RetrySendingConfirmationForNonHttpBadRequestResponseIfAdsIsDisabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a",
       {{net::HTTP_INTERNAL_SERVER_ERROR, {}}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_, OnDidSendConfirmation(_))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToSendConfirmation(*confirmation, /*should_retry*/ true));

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnDidRedeemUnblindedToken(_, _))
      .Times(0);

  EXPECT_CALL(*redeem_unblinded_token_delegate_mock_,
              OnFailedToRedeemUnblindedToken(_, _, _))
      .Times(0);

  redeem_unblinded_token_->Redeem(*confirmation);

  // Assert
}

}  // namespace ads
