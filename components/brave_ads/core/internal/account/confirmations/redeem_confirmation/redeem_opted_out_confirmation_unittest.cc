/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/redeem_confirmation/redeem_opted_out_confirmation.h"

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/redeem_confirmation/redeem_confirmation_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/redeem_confirmation/url_request_builders/create_opted_out_confirmation_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/net/http/http_status_code.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_unittest_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using ::testing::_;
using ::testing::NiceMock;

class BraveAdsRedeemOptedOutConfirmationTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    ads_client_mock_.SetBooleanPref(prefs::kEnabled, false);
  }

  NiceMock<privacy::TokenGeneratorMock> token_generator_mock_;

  NiceMock<RedeemConfirmationDelegateMock> redeem_confirmation_delegate_mock_;
  base::WeakPtrFactory<RedeemConfirmationDelegateMock>
      confirmation_delegate_weak_factory_{&redeem_confirmation_delegate_mock_};
};

TEST_F(BraveAdsRedeemOptedOutConfirmationTest, Redeem) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  const URLResponseMap url_responses = {
      {BuildCreateOptedOutConfirmationUrlPath(kTransactionId),
       {{net::kHttpImATeapot, /*response_body*/ "418 I'm a teapot"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const absl::optional<ConfirmationInfo> confirmation =
      BuildConfirmation(&token_generator_mock_);
  ASSERT_TRUE(confirmation);

  // Act
  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(*confirmation));

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(_, _, _))
      .Times(0);

  RedeemOptedOutConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedOutConfirmationTest,
       DoNotRetryRedeemingForHttpBadRequestResponse) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  const URLResponseMap url_responses = {
      {BuildCreateOptedOutConfirmationUrlPath(kTransactionId),
       {{net::HTTP_BAD_REQUEST,
         /*response_body*/ net::GetHttpReasonPhrase(net::HTTP_BAD_REQUEST)}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const absl::optional<ConfirmationInfo> confirmation =
      BuildConfirmation(&token_generator_mock_);
  ASSERT_TRUE(confirmation);

  // Act
  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(
      redeem_confirmation_delegate_mock_,
      OnFailedToRedeemConfirmation(*confirmation, /*should_retry*/ false,
                                   /*should_backoff*/ false));

  RedeemOptedOutConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedOutConfirmationTest,
       DoNotRetryRedeemingForHttpConflictResponse) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  const URLResponseMap url_responses = {
      {BuildCreateOptedOutConfirmationUrlPath(kTransactionId),
       {{net::HTTP_CONFLICT,
         /*response_body*/ net::GetHttpReasonPhrase(net::HTTP_CONFLICT)}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const absl::optional<ConfirmationInfo> confirmation =
      BuildConfirmation(&token_generator_mock_);
  ASSERT_TRUE(confirmation);

  // Act
  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(
      redeem_confirmation_delegate_mock_,
      OnFailedToRedeemConfirmation(*confirmation, /*should_retry*/ false,
                                   /*should_backoff*/ false));

  RedeemOptedOutConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedOutConfirmationTest,
       DoNotRetryReemingForHttpCreatedResponse) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  const URLResponseMap url_responses = {
      {BuildCreateOptedOutConfirmationUrlPath(kTransactionId),
       {{net::HTTP_CREATED,
         /*response_body*/ net::GetHttpReasonPhrase(net::HTTP_CREATED)}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const absl::optional<ConfirmationInfo> confirmation =
      BuildConfirmation(&token_generator_mock_);
  ASSERT_TRUE(confirmation);

  // Act
  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(
      redeem_confirmation_delegate_mock_,
      OnFailedToRedeemConfirmation(*confirmation, /*should_retry*/ false,
                                   /*should_backoff*/ false));

  RedeemOptedOutConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedOutConfirmationTest, RetryRedeeming) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  const URLResponseMap url_responses = {
      {BuildCreateOptedOutConfirmationUrlPath(kTransactionId),
       {{net::HTTP_INTERNAL_SERVER_ERROR,
         /*response_body*/ net::GetHttpReasonPhrase(
             net::HTTP_INTERNAL_SERVER_ERROR)}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const absl::optional<ConfirmationInfo> confirmation =
      BuildConfirmation(&token_generator_mock_);
  ASSERT_TRUE(confirmation);

  // Act
  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(*confirmation, /*should_retry*/ true,
                                           /*should_backoff*/ true));

  RedeemOptedOutConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

}  // namespace brave_ads
