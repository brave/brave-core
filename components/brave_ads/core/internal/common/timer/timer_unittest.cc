/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/timer/timer.h"

#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/test/bind.h"
#include "base/test/gtest_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/random/random_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTimerTest : public test::TestBase {
 protected:
  Timer timer_;
};

TEST_F(BraveAdsTimerTest, IsNotRunningInitially) {
  // Act & Assert
  EXPECT_FALSE(timer_.IsRunning());
}

TEST_F(BraveAdsTimerTest, Start) {
  // Arrange
  const ScopedTimerDelaySetterForTesting scoped_delay(base::Seconds(7));

  // Act
  timer_.Start(FROM_HERE, base::Seconds(1), base::DoNothing());

  // Assert
  EXPECT_TRUE(timer_.IsRunning());
  EXPECT_EQ(base::Seconds(7), NextPendingTaskDelay());
}

TEST_F(BraveAdsTimerTest, StartRunsCallbackWhenDelayExpires) {
  // Arrange
  const ScopedTimerDelaySetterForTesting scoped_delay(base::Seconds(7));
  bool did_fire = false;

  // Act
  timer_.Start(FROM_HERE, base::Seconds(1),
               base::BindLambdaForTesting([&] { did_fire = true; }));
  ASSERT_TRUE(timer_.IsRunning());
  ASSERT_EQ(base::Seconds(7), NextPendingTaskDelay());
  FastForwardClockBy(base::Seconds(7));

  // Assert
  EXPECT_TRUE(did_fire);
  EXPECT_FALSE(timer_.IsRunning());
}

TEST_F(BraveAdsTimerTest, StartingAgainReplacesRunningTimer) {
  // Arrange
  const ScopedTimerDelaySetterForTesting scoped_delay(base::Seconds(7));

  int fire_count = 0;
  timer_.Start(FROM_HERE, base::Seconds(1),
               base::BindLambdaForTesting([&] { ++fire_count; }));

  // Act
  timer_.Start(FROM_HERE, base::Seconds(1),
               base::BindLambdaForTesting([&] { ++fire_count; }));
  FastForwardClockBy(base::Seconds(7));

  // Assert
  EXPECT_EQ(1, fire_count);
}

TEST_F(BraveAdsTimerTest, StartWithPrivacy) {
  // Arrange
  const ScopedTimerDelaySetterForTesting scoped_delay(base::Seconds(7));

  // Act
  timer_.StartWithPrivacy(FROM_HERE, base::Seconds(1), base::DoNothing());

  // Assert
  EXPECT_TRUE(timer_.IsRunning());
}

TEST_F(BraveAdsTimerTest, StartWithPrivacyRunsCallbackWhenDelayExpires) {
  // Arrange
  const ScopedTimerDelaySetterForTesting scoped_delay(base::Seconds(7));
  bool did_fire = false;

  // Act
  timer_.StartWithPrivacy(FROM_HERE, base::Seconds(1),
                          base::BindLambdaForTesting([&] { did_fire = true; }));
  ASSERT_TRUE(timer_.IsRunning());
  FastForwardClockBy(base::Seconds(7));

  // Assert
  EXPECT_TRUE(did_fire);
  EXPECT_FALSE(timer_.IsRunning());
}

TEST_F(BraveAdsTimerTest, StopReturnsTrueIfRunning) {
  // Arrange
  timer_.Start(FROM_HERE, base::Seconds(1), base::DoNothing());

  // Act & Assert
  EXPECT_TRUE(timer_.Stop());
}

TEST_F(BraveAdsTimerTest, StopPreventsTaskFromRunning) {
  // Arrange
  const ScopedTimerDelaySetterForTesting scoped_delay(base::Seconds(7));
  bool did_fire = false;
  timer_.Start(FROM_HERE, base::Seconds(1),
               base::BindLambdaForTesting([&] { did_fire = true; }));

  // Act
  timer_.Stop();
  FastForwardClockBy(base::Seconds(7));

  // Assert
  EXPECT_FALSE(did_fire);
}

TEST_F(BraveAdsTimerTest, StopReturnsFalseIfNotRunning) {
  // Act & Assert
  EXPECT_FALSE(timer_.Stop());
}

TEST_F(BraveAdsTimerTest, StartWithPrivacyChecksNegativeDelay) {
  // Arrange
  const ScopedRandTimeDeltaWithJitterSetterForTesting scoped_rand_delay(
      base::Milliseconds(-1));

  // Act & Assert
  EXPECT_CHECK_DEATH(
      timer_.StartWithPrivacy(FROM_HERE, base::Seconds(10), base::DoNothing()));
}

}  // namespace brave_ads
