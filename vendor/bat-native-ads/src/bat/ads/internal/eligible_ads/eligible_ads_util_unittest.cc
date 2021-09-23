/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/eligible_ads_util.h"

#include <vector>

#include "base/guid.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/bundle/creative_ad_notification_unittest_util.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsEligibleAdsUtilTest,
     GroupEligibleAdsByCreativeInstanceIdForEmptyAds) {
  // Arrange
  std::vector<CreativeAdNotificationInfo> eligible_ads;

  // Act
  const CreativeAdNotificationPredictorMap ads =
      GroupEligibleAdsByCreativeInstanceId(eligible_ads);

  // Assert
  EXPECT_TRUE(ads.empty());
}

TEST(BatAdsEligibleAdsUtilTest, GroupEligibleAdsByCreativeInstanceId) {
  // Arrange
  std::vector<CreativeAdNotificationInfo> eligible_ads;

  const CreativeAdNotificationInfo creative_ad_notification_1 =
      GetCreativeAdNotification(base::GenerateGUID(), "foo-bar1");
  eligible_ads.push_back(creative_ad_notification_1);

  const CreativeAdNotificationInfo creative_ad_notification_2 =
      GetCreativeAdNotification(base::GenerateGUID(), "foo-bar2");
  eligible_ads.push_back(creative_ad_notification_2);

  const CreativeAdNotificationInfo creative_ad_notification_3 =
      GetCreativeAdNotification(base::GenerateGUID(), "foo-bar3");
  eligible_ads.push_back(creative_ad_notification_3);

  const CreativeAdNotificationInfo creative_ad_notification_4 =
      GetCreativeAdNotification(creative_ad_notification_2.creative_instance_id,
                                "foo-bar4");
  eligible_ads.push_back(creative_ad_notification_4);

  // Act
  const CreativeAdNotificationPredictorMap ads =
      GroupEligibleAdsByCreativeInstanceId(eligible_ads);

  // Assert
  ASSERT_EQ(3u, ads.size());

  AdPredictorInfo<CreativeAdNotificationInfo> ad =
      ads.at(creative_ad_notification_2.creative_instance_id);
  const SegmentList& expected_segments = {"foo-bar2", "foo-bar4"};
  EXPECT_EQ(expected_segments, ad.segments);
}

}  // namespace ads
