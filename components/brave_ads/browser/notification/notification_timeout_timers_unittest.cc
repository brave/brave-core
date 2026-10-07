/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/notification/notification_timeout_timers.h"

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_feature.h"
#include "testing/gtest/include/gtest/gtest.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNotificationTimeoutTimersTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};

  NotificationTimeoutTimers timers_;
};

TEST_F(BraveAdsNotificationTimeoutTimersTest,
       TimesOutAfterTheConfiguredTimeout) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNotificationAdFeature, {{"notification_ad_timeout", "10s"}});

  base::test::TestFuture<void> timed_out_future;

  // Act
  timers_.Start("notification_id", timed_out_future.GetCallback());
  task_environment_.FastForwardBy(base::Seconds(10));

  // Assert
  EXPECT_TRUE(timed_out_future.IsReady());
}

TEST_F(BraveAdsNotificationTimeoutTimersTest,
       DoesNotTimeOutBeforeTheConfiguredTimeout) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNotificationAdFeature, {{"notification_ad_timeout", "10s"}});

  base::test::TestFuture<void> timed_out_future;

  // Act
  timers_.Start("notification_id", timed_out_future.GetCallback());
  task_environment_.FastForwardBy(base::Seconds(9));

  // Assert
  EXPECT_FALSE(timed_out_future.IsReady());
}

TEST_F(BraveAdsNotificationTimeoutTimersTest, NeverTimesOutWhenTimeoutIsZero) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNotificationAdFeature, {{"notification_ad_timeout", "0s"}});

  base::test::TestFuture<void> timed_out_future;

  // Act
  timers_.Start("notification_id", timed_out_future.GetCallback());
  task_environment_.FastForwardBy(base::Days(1));

  // Assert
  EXPECT_FALSE(timed_out_future.IsReady());
}

TEST_F(BraveAdsNotificationTimeoutTimersTest,
       StopCancelsTheTimerAndReturnsTrue) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNotificationAdFeature, {{"notification_ad_timeout", "10s"}});

  base::test::TestFuture<void> timed_out_future;
  timers_.Start("notification_id", timed_out_future.GetCallback());

  // Act
  const bool was_stopped = timers_.Stop("notification_id");
  task_environment_.FastForwardBy(base::Seconds(10));

  // Assert
  EXPECT_TRUE(was_stopped);
  EXPECT_FALSE(timed_out_future.IsReady());
}

TEST_F(BraveAdsNotificationTimeoutTimersTest,
       StopReturnsFalseForAnUnknownNotificationId) {
  // Act & Assert
  EXPECT_FALSE(timers_.Stop("unknown_notification_id"));
}

TEST_F(BraveAdsNotificationTimeoutTimersTest, StopAllCancelsEveryTimer) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNotificationAdFeature, {{"notification_ad_timeout", "10s"}});

  base::test::TestFuture<void> timed_out_future_1;
  base::test::TestFuture<void> timed_out_future_2;
  timers_.Start("notification_id_1", timed_out_future_1.GetCallback());
  timers_.Start("notification_id_2", timed_out_future_2.GetCallback());

  // Act
  timers_.StopAll();
  task_environment_.FastForwardBy(base::Seconds(10));

  // Assert
  EXPECT_FALSE(timed_out_future_1.IsReady());
  EXPECT_FALSE(timed_out_future_2.IsReady());
}

}  // namespace brave_ads
