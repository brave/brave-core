/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/queue/conversion_queue.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/conversion_queue_delegate.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionQueueTest : public ConversionQueueDelegate,
                                    public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    conversion_queue_ = std::make_unique<ConversionQueue>();
    conversion_queue_->SetDelegate(this);
  }

  void OnDidAddConversionToQueue(const ConversionInfo& conversion) override {
    conversion_ = conversion;
    did_add_to_queue_ = true;
  }

  void OnFailedToAddConversionToQueue(
      const ConversionInfo& conversion) override {
    conversion_ = conversion;
    failed_to_add_to_queue_ = true;
  }

  void OnWillProcessConversionQueue(const ConversionInfo& /*conversion*/,
                                    base::Time process_at) override {
    // We should not set |conversion_| because otherwise, this will be called
    // when the next item in the queue is processed.

    will_process_queue_at_ = process_at;
  }

  void OnDidProcessConversionQueue(const ConversionInfo& conversion) override {
    conversion_ = conversion;
    did_process_queue_ = true;
  }

  void OnFailedToProcessConversionQueue(
      const ConversionInfo& /*conversion*/) override {
    failed_to_process_queue_ = true;
  }

  void OnFailedToProcessNextConversionInQueue() override {
    failed_to_process_next_conversion_in_queue_ = true;
  }

  void OnDidExhaustConversionQueue() override { did_exhaust_queue_ = true; }

  void ResetDelegate() {
    conversion_.reset();
    did_add_to_queue_ = false;
    failed_to_add_to_queue_ = false;
    will_process_queue_at_.reset();
    did_process_queue_ = false;
    failed_to_process_queue_ = false;
    failed_to_process_next_conversion_in_queue_ = false;
    did_exhaust_queue_ = false;
  }

  std::unique_ptr<ConversionQueue> conversion_queue_;

  absl::optional<ConversionInfo> conversion_;
  bool did_add_to_queue_ = false;
  bool failed_to_add_to_queue_ = false;
  absl::optional<base::Time> will_process_queue_at_;
  bool did_process_queue_ = false;
  bool failed_to_process_queue_ = false;
  bool failed_to_process_next_conversion_in_queue_ = false;
  bool did_exhaust_queue_ = false;
};

TEST_F(BraveAdsConversionQueueTest, AddConversionToQueue) {
  // Arrange
  const AdInfo ad =
      BuildAd(AdType::kNotificationAd, /*should_use_random_uuids*/ false);

  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now()),
      /*verifiable_conversion*/ absl::nullopt);

  const ScopedDelayBeforeProcessingConversionQueueItemForTesting
      scoped_delay_before_processing_conversion_queue_item(base::Minutes(5));

  // Act
  conversion_queue_->Add(conversion);

  // Assert
  EXPECT_TRUE(did_add_to_queue_);
  EXPECT_FALSE(failed_to_add_to_queue_);
  EXPECT_EQ(Now() + base::Minutes(5), will_process_queue_at_);
  EXPECT_EQ(conversion, conversion_);
  EXPECT_FALSE(did_process_queue_);
  EXPECT_FALSE(failed_to_process_queue_);
  EXPECT_FALSE(failed_to_process_next_conversion_in_queue_);
  EXPECT_FALSE(did_exhaust_queue_);
}

TEST_F(BraveAdsConversionQueueTest, ProcessSingleConversionInQueue) {
  // Arrange
  const AdInfo ad =
      BuildAd(AdType::kNotificationAd, /*should_use_random_uuids*/ false);

  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now()),
      /*verifiable_conversion*/ absl::nullopt);

  const ScopedDelayBeforeProcessingConversionQueueItemForTesting
      scoped_delay_before_processing_conversion_queue_item(base::Minutes(21));

  conversion_queue_->Add(conversion);

  EXPECT_TRUE(did_add_to_queue_);
  EXPECT_FALSE(failed_to_add_to_queue_);
  EXPECT_EQ(Now() + base::Minutes(21), will_process_queue_at_);
  EXPECT_EQ(conversion, conversion_);

  // Act
  FastForwardClockToNextPendingTask();

  // Assert
  ASSERT_EQ(conversion, conversion_);
  EXPECT_TRUE(did_process_queue_);
  EXPECT_FALSE(failed_to_process_queue_);
  EXPECT_FALSE(failed_to_process_next_conversion_in_queue_);
  EXPECT_TRUE(did_exhaust_queue_);
}

TEST_F(BraveAdsConversionQueueTest, ProcessMultipleConversionsInQueue) {
  // Arrange
  AdvanceClockTo(
      TimeFromString("November 18 2023 19:00:00.000", /*is_local*/ true));

  const AdInfo ad_1 =
      BuildAd(AdType::kNotificationAd, /*should_use_random_uuids*/ true);
  const ConversionInfo conversion_1 = BuildConversion(
      BuildAdEvent(ad_1, ConfirmationType::kViewed, /*created_at*/ Now()),
      /*verifiable_conversion*/ absl::nullopt);

  {
    const ScopedDelayBeforeProcessingConversionQueueItemForTesting
        scoped_delay_before_processing_conversion_queue_item(base::Minutes(7));
    conversion_queue_->Add(conversion_1);

    EXPECT_TRUE(did_add_to_queue_);
    EXPECT_FALSE(failed_to_add_to_queue_);
    EXPECT_EQ(Now() + base::Minutes(7), will_process_queue_at_);
    EXPECT_EQ(conversion_1, conversion_);

    ResetDelegate();
  }

  const AdInfo ad_2 =
      BuildAd(AdType::kSearchResultAd, /*should_use_random_uuids*/ true);
  const ConversionInfo conversion_2 = BuildConversion(
      BuildAdEvent(ad_2, ConfirmationType::kViewed, /*created_at*/ Now()),
      /*verifiable_conversion*/ absl::nullopt);

  {
    const ScopedDelayBeforeProcessingConversionQueueItemForTesting
        scoped_delay_before_processing_conversion_queue_item(base::Minutes(3));
    conversion_queue_->Add(conversion_2);  // Should be processed first.

    EXPECT_TRUE(did_add_to_queue_);
    EXPECT_FALSE(failed_to_add_to_queue_);
    EXPECT_EQ(Now() + base::Minutes(3), will_process_queue_at_);
    EXPECT_EQ(conversion_2, conversion_);

    ResetDelegate();
  }

  FastForwardClockToNextPendingTask();

  ASSERT_EQ(conversion_2, conversion_);
  EXPECT_TRUE(did_process_queue_);
  EXPECT_FALSE(failed_to_process_queue_);
  EXPECT_FALSE(failed_to_process_next_conversion_in_queue_);
  EXPECT_FALSE(did_exhaust_queue_);

  ResetDelegate();

  // Act
  FastForwardClockToNextPendingTask();

  // Assert
  ASSERT_EQ(conversion_1, conversion_);
  EXPECT_TRUE(did_process_queue_);
  EXPECT_FALSE(failed_to_process_queue_);
  EXPECT_FALSE(failed_to_process_next_conversion_in_queue_);
  EXPECT_TRUE(did_exhaust_queue_);
}

}  // namespace brave_ads
