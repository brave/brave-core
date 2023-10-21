/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/queue/conversion_queue.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/conversion_queue_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_util.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_builder.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionQueueTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    conversion_queue_ = std::make_unique<ConversionQueue>();
    conversion_queue_->SetDelegate(&delegate_mock_);
  }

  std::unique_ptr<ConversionQueue> conversion_queue_;
  ::testing::StrictMock<ConversionQueueDelegateMock> delegate_mock_;

  const ::testing::InSequence s_;
};

TEST_F(BraveAdsConversionQueueTest, AddConversionToQueue) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/false);

  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at=*/Now()),
      /*verifiable_conversion=*/absl::nullopt);

  const ScopedDelayBeforeProcessingConversionQueueItemForTesting
      scoped_delay_before_processing_conversion_queue_item(base::Minutes(5));

  EXPECT_CALL(delegate_mock_, OnDidAddConversionToQueue(conversion));
  EXPECT_CALL(delegate_mock_, OnWillProcessConversionQueue(
                                  conversion, Now() + base::Minutes(5)));

  // Act
  conversion_queue_->Add(conversion);

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(BraveAdsConversionQueueTest, ProcessConversionInQueue) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/false);

  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at=*/Now()),
      /*verifiable_conversion=*/absl::nullopt);

  EXPECT_CALL(delegate_mock_, OnDidAddConversionToQueue(conversion));
  EXPECT_CALL(delegate_mock_, OnWillProcessConversionQueue(
                                  conversion, Now() + base::Minutes(21)));

  const ScopedDelayBeforeProcessingConversionQueueItemForTesting
      scoped_delay_before_processing_conversion_queue_item(base::Minutes(21));
  conversion_queue_->Add(conversion);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidProcessConversionQueue(conversion));
  EXPECT_CALL(delegate_mock_, OnDidExhaustConversionQueue);
  FastForwardClockToNextPendingTask();
}

TEST_F(BraveAdsConversionQueueTest, ProcessMultipleConversionsInQueue) {
  // Arrange
  const AdInfo ad_1 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const ConversionInfo conversion_1 = BuildConversion(
      BuildAdEvent(ad_1, ConfirmationType::kViewed, /*created_at=*/Now()),
      /*verifiable_conversion=*/absl::nullopt);

  {
    EXPECT_CALL(delegate_mock_, OnDidAddConversionToQueue(conversion_1));
    EXPECT_CALL(delegate_mock_, OnWillProcessConversionQueue(
                                    conversion_1, Now() + base::Minutes(7)));

    const ScopedDelayBeforeProcessingConversionQueueItemForTesting
        scoped_delay_before_processing_conversion_queue_item(base::Minutes(7));
    conversion_queue_->Add(conversion_1);

    EXPECT_TRUE(::testing::Mock::VerifyAndClearExpectations(&delegate_mock_));
  }

  const AdInfo ad_2 = test::BuildAd(AdType::kSearchResultAd,
                                    /*should_use_random_uuids=*/true);
  const ConversionInfo conversion_2 = BuildConversion(
      BuildAdEvent(ad_2, ConfirmationType::kViewed, /*created_at=*/Now()),
      /*verifiable_conversion=*/absl::nullopt);

  const ScopedDelayBeforeProcessingConversionQueueItemForTesting
      scoped_delay_before_processing_conversion_queue_item(base::Minutes(21));

  {
    EXPECT_CALL(delegate_mock_, OnDidAddConversionToQueue(conversion_2));

    conversion_queue_->Add(conversion_2);

    EXPECT_TRUE(::testing::Mock::VerifyAndClearExpectations(&delegate_mock_));
  }

  EXPECT_CALL(delegate_mock_, OnDidProcessConversionQueue(conversion_1));

  EXPECT_CALL(delegate_mock_,
              OnWillProcessConversionQueue(
                  conversion_2, Now() + base::Minutes(7) + base::Minutes(21)));

  FastForwardClockToNextPendingTask();

  EXPECT_CALL(delegate_mock_, OnDidProcessConversionQueue(conversion_2));

  EXPECT_CALL(delegate_mock_, OnDidExhaustConversionQueue);

  FastForwardClockToNextPendingTask();
}

}  // namespace brave_ads
