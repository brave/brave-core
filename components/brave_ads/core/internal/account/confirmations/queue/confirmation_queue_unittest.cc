/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/non_reward/redeem_non_reward_confirmation_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/non_reward/url_request_builders/create_non_reward_confirmation_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/redeem_reward_confirmation_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/create_reward_confirmation_url_request_builder_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/create_reward_confirmation_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/fetch_payment_token_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/net/http/http_status_code.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_url_response_alias.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "net/http/http_status_code.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationQueueTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    confirmation_queue_ = std::make_unique<ConfirmationQueue>();
    confirmation_queue_->SetDelegate(&delegate_mock_);
  }

  ::testing::NiceMock<TokenGeneratorMock> token_generator_mock_;

  std::unique_ptr<ConfirmationQueue> confirmation_queue_;
  ::testing::StrictMock<ConfirmationQueueDelegateMock> delegate_mock_;

  const ::testing::InSequence s_;
};

TEST_F(BraveAdsConfirmationQueueTest, AddRewardConfirmationToQueue) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::SetConfirmationTokens(/*count=*/1);

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/true);
  const absl::optional<ConfirmationInfo> confirmation = BuildRewardConfirmation(
      &token_generator_mock_, transaction, /*user_data=*/{});
  ASSERT_TRUE(confirmation);

  ScopedTimerDelaySetterForTesting scoped_setter(base::Seconds(7));

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidAddConfirmationToQueue(*confirmation));
  EXPECT_CALL(delegate_mock_, OnWillProcessConfirmationQueue(
                                  *confirmation, Now() + base::Seconds(7)));
  confirmation_queue_->Add(*confirmation);

  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(BraveAdsConfirmationQueueTest, AddNonRewardConfirmationToQueue) {
  // Arrange
  test::DisableBraveRewards();

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/true);
  const absl::optional<ConfirmationInfo> confirmation =
      BuildNonRewardConfirmation(transaction, /*user_data=*/{});
  ASSERT_TRUE(confirmation);

  ScopedTimerDelaySetterForTesting scoped_setter(base::Seconds(7));

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidAddConfirmationToQueue(*confirmation));
  EXPECT_CALL(delegate_mock_, OnWillProcessConfirmationQueue(
                                  *confirmation, Now() + base::Seconds(7)));
  confirmation_queue_->Add(*confirmation);

  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(BraveAdsConfirmationQueueTest, ProcessRewardConfirmationInQueue) {
  // Arrange
  test::BuildAndSetIssuers();

  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  const URLResponseMap url_responses = {
      {BuildCreateRewardConfirmationUrlPath(
           kTransactionId, kCreateRewardConfirmationCredential),
       {{net::HTTP_CREATED,
         test::BuildCreateRewardConfirmationUrlResponseBody()}}},
      {BuildFetchPaymentTokenUrlPath(kTransactionId),
       {{net::HTTP_OK, test::BuildFetchPaymentTokenUrlResponseBody()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  test::SetConfirmationTokens(/*count=*/1);

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/false);
  const absl::optional<ConfirmationInfo> confirmation = BuildRewardConfirmation(
      &token_generator_mock_, transaction, /*user_data=*/{});
  ASSERT_TRUE(confirmation);

  EXPECT_CALL(delegate_mock_, OnDidAddConfirmationToQueue(*confirmation));
  EXPECT_CALL(delegate_mock_, OnWillProcessConfirmationQueue(
                                  *confirmation, Now() + base::Seconds(7)));

  ScopedTimerDelaySetterForTesting scoped_setter(base::Seconds(7));

  confirmation_queue_->Add(*confirmation);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidProcessConfirmationQueue);
  EXPECT_CALL(delegate_mock_, OnDidExhaustConfirmationQueue);
  FastForwardClockToNextPendingTask();
}

TEST_F(BraveAdsConfirmationQueueTest, ProcessNonRewardConfirmationQueue) {
  // Arrange
  test::DisableBraveRewards();

  const URLResponseMap url_responses = {
      {BuildCreateNonRewardConfirmationUrlPath(kTransactionId),
       {{net::kHttpImATeapot,
         test::BuildCreateNonRewardConfirmationUrlResponseBody()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/false);
  const absl::optional<ConfirmationInfo> confirmation =
      BuildNonRewardConfirmation(transaction,
                                 /*user_data=*/{});
  ASSERT_TRUE(confirmation);

  EXPECT_CALL(delegate_mock_, OnDidAddConfirmationToQueue(*confirmation));
  EXPECT_CALL(delegate_mock_, OnWillProcessConfirmationQueue(
                                  *confirmation, Now() + base::Seconds(7)));

  ScopedTimerDelaySetterForTesting scoped_setter(base::Seconds(7));

  confirmation_queue_->Add(*confirmation);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidProcessConfirmationQueue);
  EXPECT_CALL(delegate_mock_, OnDidExhaustConfirmationQueue);
  FastForwardClockToNextPendingTask();
}

TEST_F(BraveAdsConfirmationQueueTest,
       ProcessMultipleRewardConfirmationsInQueue) {
  // Arrange
  test::BuildAndSetIssuers();

  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  const URLResponseMap url_responses = {
      {BuildCreateRewardConfirmationUrlPath(
           kTransactionId, kCreateRewardConfirmationCredential),
       {{net::HTTP_CREATED,
         test::BuildCreateRewardConfirmationUrlResponseBody()}}},
      {BuildFetchPaymentTokenUrlPath(kTransactionId),
       {{net::HTTP_OK, test::BuildFetchPaymentTokenUrlResponseBody()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  test::SetConfirmationTokens(/*count=*/2);

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/false);
  absl::optional<ConfirmationInfo> confirmation = BuildRewardConfirmation(
      &token_generator_mock_, transaction, /*user_data=*/{});
  ASSERT_TRUE(confirmation);

  {
    EXPECT_CALL(delegate_mock_, OnDidAddConfirmationToQueue(*confirmation));
    EXPECT_CALL(delegate_mock_, OnWillProcessConfirmationQueue(
                                    *confirmation, Now() + base::Seconds(7)));

    ScopedTimerDelaySetterForTesting scoped_setter(base::Seconds(7));

    confirmation_queue_->Add(*confirmation);

    ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&delegate_mock_));
  }

  {
    EXPECT_CALL(delegate_mock_, OnDidAddConfirmationToQueue(*confirmation));

    confirmation_queue_->Add(*confirmation);

    ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&delegate_mock_));
  }

  EXPECT_CALL(delegate_mock_, OnDidProcessConfirmationQueue);

  EXPECT_CALL(delegate_mock_,
              OnWillProcessConfirmationQueue(
                  *confirmation, Now() + base::Seconds(7) + base::Seconds(21)));

  ScopedTimerDelaySetterForTesting scoped_setter(base::Seconds(21));

  FastForwardClockToNextPendingTask();

  // Act & Assert
  RemoveAllPaymentTokens();  // Force |MaybeAddPaymentToken| to succeed.

  EXPECT_CALL(delegate_mock_, OnDidProcessConfirmationQueue);
  EXPECT_CALL(delegate_mock_, OnDidExhaustConfirmationQueue);
  FastForwardClockToNextPendingTask();
}

TEST_F(BraveAdsConfirmationQueueTest,
       ProcessMultipleNonRewardConfirmationsInQueue) {
  // Arrange
  test::DisableBraveRewards();

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

  {
    EXPECT_CALL(delegate_mock_, OnDidAddConfirmationToQueue(*confirmation));
    EXPECT_CALL(delegate_mock_, OnWillProcessConfirmationQueue(
                                    *confirmation, Now() + base::Seconds(7)));

    ScopedTimerDelaySetterForTesting scoped_setter(base::Seconds(7));

    confirmation_queue_->Add(*confirmation);

    ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&delegate_mock_));
  }

  {
    EXPECT_CALL(delegate_mock_, OnDidAddConfirmationToQueue(*confirmation));

    confirmation_queue_->Add(*confirmation);

    ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&delegate_mock_));
  }

  EXPECT_CALL(delegate_mock_, OnDidProcessConfirmationQueue);

  ScopedTimerDelaySetterForTesting scoped_setter(base::Seconds(21));
  EXPECT_CALL(delegate_mock_,
              OnWillProcessConfirmationQueue(
                  *confirmation, Now() + base::Seconds(7) + base::Seconds(21)));

  FastForwardClockToNextPendingTask();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidProcessConfirmationQueue);
  EXPECT_CALL(delegate_mock_, OnDidExhaustConfirmationQueue);
  FastForwardClockToNextPendingTask();
}

}  // namespace brave_ads
