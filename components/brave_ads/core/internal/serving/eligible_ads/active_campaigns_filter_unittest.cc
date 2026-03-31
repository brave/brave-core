/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/active_campaigns_filter.h"

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/test/creative_notification_ad_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsActiveCampaignsFilterTest : public ::testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
};

TEST_F(BraveAdsActiveCampaignsFilterTest, RemoveCreativeAdForExpiredCampaign) {
  // Arrange
  CreativeNotificationAdList creative_ads;
  CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad.end_at = test::Now() - base::Days(1);
  creative_ads.push_back(creative_ad);

  // Act
  FilterInactiveCampaignCreativeAds(creative_ads);

  // Assert
  EXPECT_THAT(creative_ads, ::testing::IsEmpty());
}

TEST_F(BraveAdsActiveCampaignsFilterTest, RemoveCreativeAdForFutureCampaign) {
  // Arrange
  CreativeNotificationAdList creative_ads;
  CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad.start_at = test::Now() + base::Days(1);
  creative_ad.end_at = test::Now() + base::Days(30);
  creative_ads.push_back(creative_ad);

  // Act
  FilterInactiveCampaignCreativeAds(creative_ads);

  // Assert
  EXPECT_THAT(creative_ads, ::testing::IsEmpty());
}

TEST_F(BraveAdsActiveCampaignsFilterTest,
       KeepCreativeAdWhenNowIsExactlyAtCampaignStart) {
  // Arrange
  CreativeNotificationAdList creative_ads;
  CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad.start_at = test::Now();
  creative_ad.end_at = test::Now() + base::Days(30);
  creative_ads.push_back(creative_ad);

  // Act
  FilterInactiveCampaignCreativeAds(creative_ads);

  // Assert
  EXPECT_THAT(creative_ads, ::testing::ElementsAre(creative_ad));
}

TEST_F(BraveAdsActiveCampaignsFilterTest,
       KeepCreativeAdWhenNowIsExactlyAtCampaignEnd) {
  // Arrange
  CreativeNotificationAdList creative_ads;
  CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad.start_at = test::Now() - base::Days(30);
  creative_ad.end_at = test::Now();
  creative_ads.push_back(creative_ad);

  // Act
  FilterInactiveCampaignCreativeAds(creative_ads);

  // Assert
  EXPECT_THAT(creative_ads, ::testing::ElementsAre(creative_ad));
}

TEST_F(BraveAdsActiveCampaignsFilterTest, OnlyKeepCreativeAdForActiveCampaign) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo active_creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  active_creative_ad.start_at = test::Now() - base::Days(1);
  active_creative_ad.end_at = test::Now() + base::Days(1);
  creative_ads.push_back(active_creative_ad);

  CreativeNotificationAdInfo expired_creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  expired_creative_ad.end_at = test::Now() - base::Days(1);
  creative_ads.push_back(expired_creative_ad);

  // Act
  FilterInactiveCampaignCreativeAds(creative_ads);

  // Assert
  EXPECT_THAT(creative_ads, ::testing::ElementsAre(active_creative_ad));
}

}  // namespace brave_ads
