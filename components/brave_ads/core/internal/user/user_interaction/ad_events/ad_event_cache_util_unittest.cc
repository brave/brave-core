/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_cache_util.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdEventCacheUtilTest : public UnitTestBase {};

TEST_F(BraveAdsAdEventCacheUtilTest, RebuildAdEventCache) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  const AdEventInfo ad_event = BuildAdEvent(ad, ConfirmationType::kServed,
                                            /*created_at=*/Now());

  base::MockCallback<AdEventCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  RecordAdEvent(ad_event, callback.Get());

  ResetAdEventCache();

  // Act
  RebuildAdEventCache();

  // Assert
  const std::vector<base::Time> expected_cached_ad_events = {Now()};
  EXPECT_EQ(
      expected_cached_ad_events,
      GetCachedAdEvents(AdType::kNotificationAd, ConfirmationType::kServed));
}

TEST_F(BraveAdsAdEventCacheUtilTest, CacheAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  const AdEventInfo ad_event = BuildAdEvent(ad, ConfirmationType::kServed,
                                            /*created_at=*/Now());

  // Act
  CacheAdEvent(ad_event);

  // Assert
  const std::vector<base::Time> expected_cached_ad_events = {Now()};
  EXPECT_EQ(
      expected_cached_ad_events,
      GetCachedAdEvents(AdType::kNotificationAd, ConfirmationType::kServed));
}

TEST_F(BraveAdsAdEventCacheUtilTest, GetCachedAdEvents) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);

  const AdEventInfo ad_event_1 = BuildAdEvent(ad, ConfirmationType::kServed,
                                              /*created_at=*/Now());
  CacheAdEvent(ad_event_1);

  const AdEventInfo ad_event_2 = BuildAdEvent(ad, ConfirmationType::kViewed,
                                              /*created_at=*/Now());
  CacheAdEvent(ad_event_2);

  const AdEventInfo ad_event_3 =
      BuildAdEvent(ad, ConfirmationType::kServed,
                   /*created_at=*/Now() + base::Hours(1));
  CacheAdEvent(ad_event_3);

  // Act & Assert
  const std::vector<base::Time> expected_cached_ad_events = {
      ad_event_1.created_at, ad_event_3.created_at};
  EXPECT_EQ(
      expected_cached_ad_events,
      GetCachedAdEvents(AdType::kNotificationAd, ConfirmationType::kServed));
}

}  // namespace brave_ads
