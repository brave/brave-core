/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/random/random_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/random/test/scoped_rand_time_delta_with_jitter_for_testing.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsRandTimeDeltaWithJitterTest, ReturnsScopedValueWhenOverrideIsSet) {
  // Arrange
  const test::ScopedRandTimeDeltaWithJitterForTesting
      scoped_rand_time_delta_with_jitter(base::Seconds(7));

  // Act & Assert
  EXPECT_EQ(base::Seconds(7), RandTimeDeltaWithJitter(base::Minutes(42)));
}

TEST(BraveAdsRandTimeDeltaWithJitterTest,
     ReturnsValueInExpectedRangeWithoutOverride) {
  // Act
  const base::TimeDelta time_delta =
      RandTimeDeltaWithJitter(base::Seconds(100));

  // Assert
  EXPECT_GE(time_delta, base::Seconds(50));
  EXPECT_LT(time_delta, base::Seconds(150));
}

TEST(BraveAdsRandTimeDeltaWithJitterTest,
     ScopedOverrideIsRestoredAfterDestruction) {
  // Arrange
  {
    const test::ScopedRandTimeDeltaWithJitterForTesting
        scoped_rand_time_delta_with_jitter(base::Seconds(7));
  }

  // Act
  const base::TimeDelta time_delta =
      RandTimeDeltaWithJitter(base::Seconds(100));

  // Assert
  EXPECT_GE(time_delta, base::Seconds(50));
  EXPECT_LT(time_delta, base::Seconds(150));
}

}  // namespace brave_ads
