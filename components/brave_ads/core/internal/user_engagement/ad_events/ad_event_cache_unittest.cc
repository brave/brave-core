/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/user_engagement/ad_events/ad_event_cache.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kID1[] = "26330bea-9b8c-4cd3-b04a-1c74cbdf701e";
constexpr char kID2[] = "5b2f108c-e176-4a3e-8e7c-fe67fb3db518";

}  // namespace

class BraveAdsAdEventCacheTest : public UnitTestBase {
 protected:
  void CacheAdEvent(const std::string& id,
                    const AdType ad_type,
                    const ConfirmationType confirmation_type) {
    ad_event_cache_.AddEntryForInstanceId(id, ToString(ad_type),
                                          ToString(confirmation_type), Now());
  }

  std::vector<base::Time> GetCachedAdEvents(
      const AdType ad_type,
      const ConfirmationType confirmation_type) {
    return ad_event_cache_.Get(ToString(ad_type), ToString(confirmation_type));
  }

  AdEventCache ad_event_cache_;
};

TEST_F(BraveAdsAdEventCacheTest, CacheAdEventForNewType) {
  // Arrange
  CacheAdEvent(kID1, AdType::kNotificationAd, ConfirmationType::kViewed);

  // Act & Assert
  const std::vector<base::Time> expected_cached_ad_events = {Now()};
  EXPECT_EQ(
      expected_cached_ad_events,
      GetCachedAdEvents(AdType::kNotificationAd, ConfirmationType::kViewed));
}

TEST_F(BraveAdsAdEventCacheTest, CacheAdEventForExistingType) {
  // Arrange
  CacheAdEvent(kID1, AdType::kNotificationAd, ConfirmationType::kViewed);
  CacheAdEvent(kID1, AdType::kNotificationAd, ConfirmationType::kViewed);

  // Act & Assert
  const std::vector<base::Time> expected_cached_ad_events = {Now(), Now()};
  EXPECT_EQ(
      expected_cached_ad_events,
      GetCachedAdEvents(AdType::kNotificationAd, ConfirmationType::kViewed));
}

TEST_F(BraveAdsAdEventCacheTest, CacheAdEventForMultipleIds) {
  // Arrange
  CacheAdEvent(kID1, AdType::kNotificationAd, ConfirmationType::kViewed);
  CacheAdEvent(kID2, AdType::kNotificationAd, ConfirmationType::kViewed);

  // Act & Assert
  const std::vector<base::Time> expected_cached_ad_events = {Now(), Now()};
  EXPECT_EQ(
      expected_cached_ad_events,
      GetCachedAdEvents(AdType::kNotificationAd, ConfirmationType::kViewed));
}

TEST_F(BraveAdsAdEventCacheTest, CacheAdEventForMultipleAdTypes) {
  // Arrange
  CacheAdEvent(kID1, AdType::kNotificationAd, ConfirmationType::kViewed);
  CacheAdEvent(kID1, AdType::kNewTabPageAd, ConfirmationType::kClicked);

  // Act & Assert
  const std::vector<base::Time> expected_cached_ad_events = {Now()};
  EXPECT_EQ(
      expected_cached_ad_events,
      GetCachedAdEvents(AdType::kNotificationAd, ConfirmationType::kViewed));
}

TEST_F(BraveAdsAdEventCacheTest, PurgeCacheOlderThan) {
  // Arrange
  CacheAdEvent(kID1, AdType::kNotificationAd, ConfirmationType::kViewed);

  AdvanceClockBy(base::Days(1) + base::Milliseconds(1));

  CacheAdEvent(kID1, AdType::kNotificationAd, ConfirmationType::kViewed);

  // Act & Assert
  const std::vector<base::Time> expected_cached_ad_events = {Now()};
  EXPECT_EQ(
      expected_cached_ad_events,
      GetCachedAdEvents(AdType::kNotificationAd, ConfirmationType::kViewed));
}

}  // namespace brave_ads
