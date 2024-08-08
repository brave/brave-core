/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_test_constants.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/non_reward/redeem_non_reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/non_reward/url_request_builders/create_non_reward_confirmation_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/redeem_reward_confirmation_feature.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/redeem_reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/create_reward_confirmation_url_request_builder_test_constants.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/create_reward_confirmation_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/fetch_payment_token_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/net/http/http_status_code.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationQueueTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    confirmation_queue_ = std::make_unique<ConfirmationQueue>();
    confirmation_queue_->SetDelegate(&delegate_mock_);
  }

  std::unique_ptr<ConfirmationQueue> confirmation_queue_;
  ::testing::StrictMock<ConfirmationQueueDelegateMock> delegate_mock_;

  const ::testing::InSequence s_;
};

TEST_F(BraveAdsConfirmationQueueTest, AddConfirmation) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  EXPECT_CALL(delegate_mock_, OnDidAddConfirmationToQueue(*confirmation));
  EXPECT_CALL(delegate_mock_, OnWillProcessConfirmationQueue(
                                  *confirmation, /*process_at=*/test::Now() +
                                                     base::Minutes(5)));

  const ScopedDelayBeforeProcessingConfirmationQueueItemForTesting
      scoped_delay_before_processing_confirmation_queue_item(base::Minutes(5));

  // Act
  confirmation_queue_->Add(*confirmation);

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(BraveAdsConfirmationQueueTest, ProcessConfirmation) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kRedeemRewardConfirmationFeature, {{"fetch_payment_token_after", "0s"}});

  test::BuildAndSetIssuers();

  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  const test::URLResponseMap url_responses = {
      {BuildCreateRewardConfirmationUrlPath(test::kTransactionId,
                                            test::kCredentialBase64Url),
       {{net::HTTP_CREATED,
         test::BuildCreateRewardConfirmationUrlResponseBody()}}},
      {BuildFetchPaymentTokenUrlPath(test::kTransactionId),
       {{net::HTTP_OK, test::BuildFetchPaymentTokenUrlResponseBody()}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  EXPECT_CALL(delegate_mock_, OnDidAddConfirmationToQueue(*confirmation));
  EXPECT_CALL(delegate_mock_, OnWillProcessConfirmationQueue(
                                  *confirmation, /*process_at=*/test::Now() +
                                                     base::Minutes(21)));

  const ScopedDelayBeforeProcessingConfirmationQueueItemForTesting
      scoped_delay_before_processing_confirmation_queue_item(base::Minutes(21));
  confirmation_queue_->Add(*confirmation);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidProcessConfirmationQueue);
  EXPECT_CALL(delegate_mock_, OnDidExhaustConfirmationQueue);
  FastForwardClockToNextPendingTask();
}

TEST_F(BraveAdsConfirmationQueueTest, ProcessMultipleConfirmations) {
  // Arrange
  test::DisableBraveRewards();

  const std::optional<ConfirmationInfo> confirmation_1 =
      test::BuildNonRewardConfirmation(/*should_generate_random_uuids=*/true);
  ASSERT_TRUE(confirmation_1);
  {
    EXPECT_CALL(delegate_mock_, OnDidAddConfirmationToQueue(*confirmation_1));
    EXPECT_CALL(
        delegate_mock_,
        OnWillProcessConfirmationQueue(
            *confirmation_1, /*process_at=*/test::Now() + base::Minutes(7)));

    const ScopedDelayBeforeProcessingConfirmationQueueItemForTesting
        scoped_delay_before_processing_confirmation_queue_item(
            base::Minutes(7));
    confirmation_queue_->Add(*confirmation_1);

    EXPECT_TRUE(::testing::Mock::VerifyAndClearExpectations(&delegate_mock_));
  }

  const ScopedDelayBeforeProcessingConfirmationQueueItemForTesting
      scoped_delay_before_processing_confirmation_queue_item(base::Minutes(21));
  const std::optional<ConfirmationInfo> confirmation_2 =
      test::BuildNonRewardConfirmation(/*should_generate_random_uuids=*/true);
  ASSERT_TRUE(confirmation_2);
  {
    EXPECT_CALL(delegate_mock_, OnDidAddConfirmationToQueue(*confirmation_2));

    confirmation_queue_->Add(*confirmation_2);

    EXPECT_TRUE(::testing::Mock::VerifyAndClearExpectations(&delegate_mock_));
  }

  const test::URLResponseMap url_responses = {
      {BuildCreateNonRewardConfirmationUrlPath(confirmation_1->transaction_id),
       {{net::kHttpImATeapot,
         test::BuildCreateNonRewardConfirmationUrlResponseBody()}}},
      {BuildCreateNonRewardConfirmationUrlPath(confirmation_2->transaction_id),
       {{net::kHttpImATeapot,
         test::BuildCreateNonRewardConfirmationUrlResponseBody()}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidProcessConfirmationQueue(*confirmation_1));

  EXPECT_CALL(delegate_mock_, OnWillProcessConfirmationQueue(
                                  *confirmation_2,
                                  /*process_at=*/test::Now() +
                                      base::Minutes(7) + base::Minutes(21)));

  FastForwardClockToNextPendingTask();

  EXPECT_CALL(delegate_mock_, OnDidProcessConfirmationQueue(*confirmation_2));

  EXPECT_CALL(delegate_mock_, OnDidExhaustConfirmationQueue);

  FastForwardClockToNextPendingTask();
}

}  // namespace brave_ads
