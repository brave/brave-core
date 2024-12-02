/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_util.h"

#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdEventUtilTest : public test::TestBase {};

TEST_F(BraveAdsAdEventUtilTest, ToHistoryIfNoAdEvents) {
  // Arrange
  AdEventList ad_events;

  // Act
  const std::vector<base::Time> history = ToHistory(ad_events);

  // Assert
  EXPECT_THAT(history, ::testing::IsEmpty());
}

TEST_F(BraveAdsAdEventUtilTest, ToHistory) {
  // Arrange
  const CreativeAdInfo creative_ad =
      test::BuildCreativeAd(/*should_generate_random_uuids=*/true);

  AdEventList ad_events;

  const AdEventInfo ad_event_1 = test::BuildAdEvent(
      creative_ad, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kConversion,
      /*created_at=*/test::Now(), /*should_generate_random_uuids=*/true);
  ad_events.push_back(ad_event_1);

  AdvanceClockBy(base::Minutes(1));

  const AdEventInfo ad_event_2 = test::BuildAdEvent(
      creative_ad, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kConversion,
      /*created_at=*/test::Now(), /*should_generate_random_uuids=*/true);
  ad_events.push_back(ad_event_2);

  // Act
  const std::vector<base::Time> history = ToHistory(ad_events);

  // Assert
  const std::vector<base::Time> expected_history = {*ad_event_1.created_at,
                                                    *ad_event_2.created_at};
  EXPECT_EQ(expected_history, history);
}

}  // namespace brave_ads
