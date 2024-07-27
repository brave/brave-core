/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/non_reward/redeem_non_reward_confirmation.h"

#include <optional>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_test_constants.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/non_reward/redeem_non_reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/non_reward/url_request_builders/create_non_reward_confirmation_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/redeem_confirmation_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/common/net/http/http_status_code.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRedeemNonRewardConfirmationTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    test::DisableBraveRewards();
  }

  RedeemConfirmationDelegateMock delegate_mock_;
  base::WeakPtrFactory<RedeemConfirmationDelegateMock>
      confirmation_delegate_weak_factory_{&delegate_mock_};
};

TEST_F(BraveAdsRedeemNonRewardConfirmationTest, Redeem) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildCreateNonRewardConfirmationUrlPath(test::kTransactionId),
       {{net::kHttpImATeapot,
         test::BuildCreateNonRewardConfirmationUrlResponseBody()}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildNonRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRedeemConfirmation(*confirmation));

  EXPECT_CALL(delegate_mock_, OnFailedToRedeemConfirmation).Times(0);

  RedeemNonRewardConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);
}

TEST_F(BraveAdsRedeemNonRewardConfirmationTest,
       DoNotRetryRedeemingForHttpBadRequestResponse) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildCreateNonRewardConfirmationUrlPath(test::kTransactionId),
       {{net::HTTP_BAD_REQUEST,
         /*response_body=*/net::GetHttpReasonPhrase(net::HTTP_BAD_REQUEST)}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildNonRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRedeemConfirmation).Times(0);

  EXPECT_CALL(delegate_mock_, OnFailedToRedeemConfirmation(
                                  *confirmation, /*should_retry=*/false));

  RedeemNonRewardConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);
}

TEST_F(BraveAdsRedeemNonRewardConfirmationTest,
       DoNotRetryRedeemingForHttpConflictResponse) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildCreateNonRewardConfirmationUrlPath(test::kTransactionId),
       {{net::HTTP_CONFLICT,
         /*response_body=*/net::GetHttpReasonPhrase(net::HTTP_CONFLICT)}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildNonRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRedeemConfirmation).Times(0);

  EXPECT_CALL(delegate_mock_, OnFailedToRedeemConfirmation(
                                  *confirmation, /*should_retry=*/false));

  RedeemNonRewardConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);
}

TEST_F(BraveAdsRedeemNonRewardConfirmationTest,
       DoNotRetryReemingForHttpCreatedResponse) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildCreateNonRewardConfirmationUrlPath(test::kTransactionId),
       {{net::HTTP_CREATED,
         /*response_body=*/net::GetHttpReasonPhrase(net::HTTP_CREATED)}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildNonRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRedeemConfirmation).Times(0);

  EXPECT_CALL(delegate_mock_, OnFailedToRedeemConfirmation(
                                  *confirmation, /*should_retry=*/false));

  RedeemNonRewardConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);
}

TEST_F(BraveAdsRedeemNonRewardConfirmationTest, RetryRedeeming) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildCreateNonRewardConfirmationUrlPath(test::kTransactionId),
       {{net::HTTP_INTERNAL_SERVER_ERROR,
         /*response_body=*/net::GetHttpReasonPhrase(
             net::HTTP_INTERNAL_SERVER_ERROR)}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildNonRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRedeemConfirmation).Times(0);

  EXPECT_CALL(delegate_mock_, OnFailedToRedeemConfirmation(
                                  *confirmation, /*should_retry=*/true));

  RedeemNonRewardConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);
}

}  // namespace brave_ads
