/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/non_reward/redeem_non_reward_confirmation.h"

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/non_reward/redeem_non_reward_confirmation_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/non_reward/url_request_builders/create_non_reward_confirmation_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/redeem_confirmation_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/common/net/http/http_status_code.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRedeemNonRewardConfirmationTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    test::DisableBraveRewards();
  }

  RedeemConfirmationDelegateMock delegate_mock_;
  base::WeakPtrFactory<RedeemConfirmationDelegateMock>
      confirmation_delegate_weak_factory_{&delegate_mock_};
};

TEST_F(BraveAdsRedeemNonRewardConfirmationTest, Redeem) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildCreateNonRewardConfirmationUrlPath(kTransactionId),
       {{net::kHttpImATeapot,
         test::BuildCreateNonRewardConfirmationUrlResponseBody()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/false);
  const absl::optional<ConfirmationInfo> confirmation =
      BuildNonRewardConfirmation(transaction, /*user_data=*/{});
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
  const URLResponseMap url_responses = {
      {BuildCreateNonRewardConfirmationUrlPath(kTransactionId),
       {{net::HTTP_BAD_REQUEST,
         /*response_body=*/net::GetHttpReasonPhrase(net::HTTP_BAD_REQUEST)}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/false);
  const absl::optional<ConfirmationInfo> confirmation =
      BuildNonRewardConfirmation(transaction, /*user_data=*/{});
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
  const URLResponseMap url_responses = {
      {BuildCreateNonRewardConfirmationUrlPath(kTransactionId),
       {{net::HTTP_CONFLICT,
         /*response_body=*/net::GetHttpReasonPhrase(net::HTTP_CONFLICT)}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/false);
  const absl::optional<ConfirmationInfo> confirmation =
      BuildNonRewardConfirmation(transaction, /*user_data=*/{});
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
  const URLResponseMap url_responses = {
      {BuildCreateNonRewardConfirmationUrlPath(kTransactionId),
       {{net::HTTP_CREATED,
         /*response_body=*/net::GetHttpReasonPhrase(net::HTTP_CREATED)}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/false);
  const absl::optional<ConfirmationInfo> confirmation =
      BuildNonRewardConfirmation(transaction, /*user_data=*/{});
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
  const URLResponseMap url_responses = {
      {BuildCreateNonRewardConfirmationUrlPath(kTransactionId),
       {{net::HTTP_INTERNAL_SERVER_ERROR,
         /*response_body=*/net::GetHttpReasonPhrase(
             net::HTTP_INTERNAL_SERVER_ERROR)}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/false);
  const absl::optional<ConfirmationInfo> confirmation =
      BuildNonRewardConfirmation(transaction, /*user_data=*/{});
  ASSERT_TRUE(confirmation);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidRedeemConfirmation).Times(0);

  EXPECT_CALL(delegate_mock_, OnFailedToRedeemConfirmation(
                                  *confirmation, /*should_retry=*/true));

  RedeemNonRewardConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);
}

}  // namespace brave_ads
