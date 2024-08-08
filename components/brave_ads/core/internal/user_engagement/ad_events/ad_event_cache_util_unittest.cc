/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_cache_util.h"

#include <vector>

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdEventCacheUtilTest : public test::TestBase {};

TEST_F(BraveAdsAdEventCacheUtilTest, RebuildAdEventCache) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());

  base::MockCallback<AdEventCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  RecordAdEvent(ad_event, callback.Get());

  ResetAdEventCache();

  // Act
  RebuildAdEventCache();

  // Assert
  const std::vector<base::Time> expected_cached_ad_events = {test::Now()};
  EXPECT_EQ(expected_cached_ad_events,
            GetCachedAdEvents(AdType::kNotificationAd,
                              ConfirmationType::kServedImpression));
}

TEST_F(BraveAdsAdEventCacheUtilTest, CacheAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());

  // Act
  CacheAdEvent(ad_event);

  // Assert
  const std::vector<base::Time> expected_cached_ad_events = {test::Now()};
  EXPECT_EQ(expected_cached_ad_events,
            GetCachedAdEvents(AdType::kNotificationAd,
                              ConfirmationType::kServedImpression));
}

TEST_F(BraveAdsAdEventCacheUtilTest, GetCachedAdEvents) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);

  const AdEventInfo ad_event_1 =
      BuildAdEvent(ad, ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());
  ASSERT_TRUE(ad_event_1.created_at);
  CacheAdEvent(ad_event_1);

  const AdEventInfo ad_event_2 =
      BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());
  ASSERT_TRUE(ad_event_2.created_at);
  CacheAdEvent(ad_event_2);

  const AdEventInfo ad_event_3 =
      BuildAdEvent(ad, ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now() + base::Hours(1));
  ASSERT_TRUE(ad_event_3.created_at);
  CacheAdEvent(ad_event_3);

  // Act
  const std::vector<base::Time> cached_ad_events = GetCachedAdEvents(
      AdType::kNotificationAd, ConfirmationType::kServedImpression);

  // Assert
  const std::vector<base::Time> expected_cached_ad_events = {
      *ad_event_1.created_at, *ad_event_3.created_at};
  EXPECT_EQ(expected_cached_ad_events, cached_ad_events);
}

}  // namespace brave_ads
