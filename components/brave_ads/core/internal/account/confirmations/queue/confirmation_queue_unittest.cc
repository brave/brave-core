/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue.h"

#include <memory>

#include "base/run_loop.h"
#include "base/test/gmock_callback_support.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/non_reward/redeem_non_reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/non_reward/url_request_builders/create_non_reward_confirmation_url_request_builder_util.h"
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

    confirmation_queue_ = std::make_unique<ConfirmationQueue>(ads_client_mock_);
    confirmation_queue_->SetDelegate(&delegate_mock_);
  }

  std::unique_ptr<ConfirmationQueue> confirmation_queue_;
  ::testing::StrictMock<ConfirmationQueueDelegateMock> delegate_mock_;

  const ::testing::InSequence s_;
};

// Verifies that adding a non-Reward confirmation enqueues it successfully,
// schedules it for delayed processing, and leaves a pending task to process the
// queued confirmation later.
TEST_F(BraveAdsConfirmationQueueTest, AddNonRewardConfirmation) {
  // Arrange
  test::DisableBraveRewards();

  std::optional<ConfirmationInfo> confirmation =
      test::BuildNonRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  base::RunLoop run_loop_add;
  EXPECT_CALL(delegate_mock_, OnDidAddConfirmationToQueue(*confirmation))
      .WillOnce(base::test::RunOnceClosure(run_loop_add.QuitClosure()));
  base::RunLoop run_loop_process;
  EXPECT_CALL(delegate_mock_,
              OnWillProcessConfirmationQueue(
                  *confirmation, /*process_at=*/test::Now() + base::Minutes(5)))
      .WillOnce(base::test::RunOnceClosure(run_loop_process.QuitClosure()));

  const ScopedDelayBeforeProcessingConfirmationQueueItemForTesting
      scoped_delay_before_processing_confirmation_queue_item(base::Minutes(5));

  // Act
  confirmation_queue_->Add(*confirmation);
  run_loop_add.Run();
  run_loop_process.Run();

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

// Verifies that adding a Reward confirmation enqueues it successfully,
// schedules it for delayed processing, and leaves a pending task to process the
// queued confirmation later.
TEST_F(BraveAdsConfirmationQueueTest, AddRewardConfirmation) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  base::RunLoop run_loop_add;
  EXPECT_CALL(delegate_mock_, OnDidAddConfirmationToQueue(*confirmation))
      .WillOnce(base::test::RunOnceClosure(run_loop_add.QuitClosure()));
  base::RunLoop run_loop_process;
  EXPECT_CALL(delegate_mock_,
              OnWillProcessConfirmationQueue(
                  *confirmation, /*process_at=*/test::Now() + base::Minutes(5)))
      .WillOnce(base::test::RunOnceClosure(run_loop_process.QuitClosure()));

  const ScopedDelayBeforeProcessingConfirmationQueueItemForTesting
      scoped_delay_before_processing_confirmation_queue_item(base::Minutes(5));

  // Act
  confirmation_queue_->Add(*confirmation);
  run_loop_add.Run();
  run_loop_process.Run();

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

// Verifies that adding a non-Reward confirmation schedules it for delayed
// processing, triggers the appropriate delegate callbacks, and that advancing
// time executes the queued work until the confirmation queue is fully
// processed/exhausted. Brave Rewards must be disabled for this test because
// processing Reward confirmation queue items rebuilds the confirmation dynamic
// user data, causing the credential embedded in the URL to change and no longer
// match the mocked request.
TEST_F(BraveAdsConfirmationQueueTest, ProcessNonRewardConfirmation) {
  // Arrange
  test::DisableBraveRewards();

  std::optional<ConfirmationInfo> confirmation =
      test::BuildNonRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  const test::URLResponseMap url_responses = {
      {BuildCreateNonRewardConfirmationUrlPath(confirmation->transaction_id),
       {{net::HTTP_CREATED,
         test::BuildCreateNonRewardConfirmationUrlResponseBody()}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  base::RunLoop run_loop_add;
  EXPECT_CALL(delegate_mock_, OnDidAddConfirmationToQueue(*confirmation))
      .WillOnce(base::test::RunOnceClosure(run_loop_add.QuitClosure()));
  base::RunLoop run_loop_process;
  EXPECT_CALL(delegate_mock_, OnWillProcessConfirmationQueue(
                                  *confirmation, /*process_at=*/test::Now() +
                                                     base::Minutes(21)))
      .WillOnce(base::test::RunOnceClosure(run_loop_process.QuitClosure()));

  const ScopedDelayBeforeProcessingConfirmationQueueItemForTesting
      scoped_delay_before_processing_confirmation_queue_item(base::Minutes(21));
  confirmation_queue_->Add(*confirmation);
  run_loop_add.Run();
  run_loop_process.Run();

  // Act & Assert
  base::RunLoop run_loop_did_process;
  EXPECT_CALL(delegate_mock_, OnDidProcessConfirmationQueue)
      .WillOnce(base::test::RunOnceClosure(run_loop_did_process.QuitClosure()));
  EXPECT_CALL(delegate_mock_, OnDidExhaustConfirmationQueue)
      .WillOnce(base::test::RunOnceClosure(run_loop_did_process.QuitClosure()));
  FastForwardClockToNextPendingTask();
  run_loop_did_process.Run();
}

// Verifies that multiple non-reward confirmations added at different times are
// processed sequentially in the expected order, with each item scheduled using
// its configured delay, and that the queue reports completion once all
// confirmations have been processed/exhausted. Brave Rewards must be disabled
// for this test because processing Reward confirmation queue items rebuilds the
// confirmation dynamic user data, causing the credential embedded in the URL to
// change and no longer match the mocked request.
TEST_F(BraveAdsConfirmationQueueTest, ProcessMultipleNonRewardConfirmations) {
  // Arrange
  test::DisableBraveRewards();

  std::optional<ConfirmationInfo> confirmation_1 =
      test::BuildNonRewardConfirmation(/*should_generate_random_uuids=*/true);
  ASSERT_TRUE(confirmation_1);
  {
    base::RunLoop run_loop_add_1;
    EXPECT_CALL(delegate_mock_, OnDidAddConfirmationToQueue(*confirmation_1))
        .WillOnce(base::test::RunOnceClosure(run_loop_add_1.QuitClosure()));
    base::RunLoop run_loop_process_1;
    EXPECT_CALL(
        delegate_mock_,
        OnWillProcessConfirmationQueue(
            *confirmation_1, /*process_at=*/test::Now() + base::Minutes(7)))
        .WillOnce(base::test::RunOnceClosure(run_loop_process_1.QuitClosure()));

    const ScopedDelayBeforeProcessingConfirmationQueueItemForTesting
        scoped_delay_before_processing_confirmation_queue_item(
            base::Minutes(7));
    confirmation_queue_->Add(*confirmation_1);
    run_loop_add_1.Run();
    run_loop_process_1.Run();

    EXPECT_TRUE(::testing::Mock::VerifyAndClearExpectations(&delegate_mock_));
  }

  const ScopedDelayBeforeProcessingConfirmationQueueItemForTesting
      scoped_delay_before_processing_confirmation_queue_item(base::Minutes(21));
  std::optional<ConfirmationInfo> confirmation_2 =
      test::BuildNonRewardConfirmation(/*should_generate_random_uuids=*/true);
  ASSERT_TRUE(confirmation_2);
  {
    base::RunLoop run_loop_add_2;
    EXPECT_CALL(delegate_mock_, OnDidAddConfirmationToQueue(*confirmation_2))
        .WillOnce(base::test::RunOnceClosure(run_loop_add_2.QuitClosure()));

    confirmation_queue_->Add(*confirmation_2);
    run_loop_add_2.Run();

    EXPECT_TRUE(::testing::Mock::VerifyAndClearExpectations(&delegate_mock_));
  }

  const test::URLResponseMap url_responses = {
      {BuildCreateNonRewardConfirmationUrlPath(confirmation_1->transaction_id),
       {{net::HTTP_NO_CONTENT,
         test::BuildCreateNonRewardConfirmationUrlResponseBody()}}},
      {BuildCreateNonRewardConfirmationUrlPath(confirmation_2->transaction_id),
       {{net::HTTP_NO_CONTENT,
         test::BuildCreateNonRewardConfirmationUrlResponseBody()}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act & Assert
  base::RunLoop run_loop_process_1;
  EXPECT_CALL(delegate_mock_, OnDidProcessConfirmationQueue(*confirmation_1))
      .WillOnce(base::test::RunOnceClosure(run_loop_process_1.QuitClosure()));
  EXPECT_CALL(delegate_mock_, OnWillProcessConfirmationQueue(
                                  *confirmation_2,
                                  /*process_at=*/test::Now() +
                                      base::Minutes(7) + base::Minutes(21)))
      .WillOnce(base::test::RunOnceClosure(run_loop_process_1.QuitClosure()));
  FastForwardClockToNextPendingTask();
  run_loop_process_1.Run();

  base::RunLoop run_loop_process_2;
  EXPECT_CALL(delegate_mock_, OnDidProcessConfirmationQueue(*confirmation_2))
      .WillOnce(base::test::RunOnceClosure(run_loop_process_2.QuitClosure()));
  EXPECT_CALL(delegate_mock_, OnDidExhaustConfirmationQueue)
      .WillOnce(base::test::RunOnceClosure(run_loop_process_2.QuitClosure()));
  FastForwardClockToNextPendingTask();
  run_loop_process_2.Run();
}

}  // namespace brave_ads
