/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/user_engagement/ad_events/ad_event_cache.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kID1[] = "26330bea-9b8c-4cd3-b04a-1c74cbdf701e";
constexpr char kID2[] = "5b2f108c-e176-4a3e-8e7c-fe67fb3db518";

}  // namespace

class BraveAdsAdEventCacheTest : public test::TestBase {
 protected:
  void CacheAdEvent(const std::string& id,
                    const mojom::AdType mojom_ad_type,
                    const mojom::ConfirmationType mojom_confirmation_type) {
    ad_event_cache_.AddEntryForInstanceId(id, mojom_ad_type,
                                          mojom_confirmation_type,
                                          /*time=*/test::Now());
  }

  std::vector<base::Time> GetCachedAdEvents(
      const mojom::AdType mojom_ad_type,
      const mojom::ConfirmationType mojom_confirmation_type) {
    return ad_event_cache_.Get(mojom_ad_type, mojom_confirmation_type);
  }

  AdEventCache ad_event_cache_;
};

TEST_F(BraveAdsAdEventCacheTest, CacheAdEventForNewType) {
  // Arrange
  CacheAdEvent(kID1, mojom::AdType::kNotificationAd,
               mojom::ConfirmationType::kViewedImpression);

  // Act
  const std::vector<base::Time> cached_ad_events =
      GetCachedAdEvents(mojom::AdType::kNotificationAd,
                        mojom::ConfirmationType::kViewedImpression);

  // Assert
  const std::vector<base::Time> expected_cached_ad_events = {test::Now()};
  EXPECT_EQ(expected_cached_ad_events, cached_ad_events);
}

TEST_F(BraveAdsAdEventCacheTest, CacheAdEventForExistingType) {
  // Arrange
  CacheAdEvent(kID1, mojom::AdType::kNotificationAd,
               mojom::ConfirmationType::kViewedImpression);
  CacheAdEvent(kID1, mojom::AdType::kNotificationAd,
               mojom::ConfirmationType::kViewedImpression);

  // Act
  const std::vector<base::Time> cached_ad_events =
      GetCachedAdEvents(mojom::AdType::kNotificationAd,
                        mojom::ConfirmationType::kViewedImpression);

  // Assert
  const std::vector<base::Time> expected_cached_ad_events = {test::Now(),
                                                             test::Now()};
  EXPECT_EQ(expected_cached_ad_events, cached_ad_events);
}

TEST_F(BraveAdsAdEventCacheTest, CacheAdEventForMultipleIds) {
  // Arrange
  CacheAdEvent(kID1, mojom::AdType::kNotificationAd,
               mojom::ConfirmationType::kViewedImpression);
  CacheAdEvent(kID2, mojom::AdType::kNotificationAd,
               mojom::ConfirmationType::kViewedImpression);

  // Act
  const std::vector<base::Time> cached_ad_events =
      GetCachedAdEvents(mojom::AdType::kNotificationAd,
                        mojom::ConfirmationType::kViewedImpression);

  // Assert
  const std::vector<base::Time> expected_cached_ad_events = {test::Now(),
                                                             test::Now()};
  EXPECT_EQ(expected_cached_ad_events, cached_ad_events);
}

TEST_F(BraveAdsAdEventCacheTest, CacheAdEventForMultipleAdTypes) {
  // Arrange
  CacheAdEvent(kID1, mojom::AdType::kNotificationAd,
               mojom::ConfirmationType::kViewedImpression);
  CacheAdEvent(kID1, mojom::AdType::kNewTabPageAd,
               mojom::ConfirmationType::kClicked);

  // Act
  const std::vector<base::Time> cached_ad_events =
      GetCachedAdEvents(mojom::AdType::kNotificationAd,
                        mojom::ConfirmationType::kViewedImpression);

  // Assert
  const std::vector<base::Time> expected_cached_ad_events = {test::Now()};
  EXPECT_EQ(expected_cached_ad_events, cached_ad_events);
}

TEST_F(BraveAdsAdEventCacheTest, PurgeCacheOlderThan) {
  // Arrange
  CacheAdEvent(kID1, mojom::AdType::kNotificationAd,
               mojom::ConfirmationType::kViewedImpression);

  AdvanceClockBy(base::Days(1) + base::Milliseconds(1));

  CacheAdEvent(kID1, mojom::AdType::kNotificationAd,
               mojom::ConfirmationType::kViewedImpression);

  // Act
  const std::vector<base::Time> cached_ad_events =
      GetCachedAdEvents(mojom::AdType::kNotificationAd,
                        mojom::ConfirmationType::kViewedImpression);

  // Assert
  const std::vector<base::Time> expected_cached_ad_events = {test::Now()};
  EXPECT_EQ(expected_cached_ad_events, cached_ad_events);
}

}  // namespace brave_ads
