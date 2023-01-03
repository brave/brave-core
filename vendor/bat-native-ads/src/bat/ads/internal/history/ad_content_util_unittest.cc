/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/history/ad_content_util.h"

#include "bat/ads/ad_content_action_types.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "bat/ads/internal/creatives/notification_ads/notification_ad_builder.h"
#include "bat/ads/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kTitle[] = "title";
constexpr char kDescription[] = "description";

}  // namespace

class BatAdsAdContentUtilTest : public UnitTestBase {};

TEST_F(BatAdsAdContentUtilTest, Build) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  // Act
  const AdContentInfo ad_content =
      BuildAdContent(ad, ConfirmationType::kViewed, kTitle, kDescription);

  // Assert
  AdContentInfo expected_ad_content;
  expected_ad_content.type = ad.type;
  expected_ad_content.placement_id = ad.placement_id;
  expected_ad_content.creative_instance_id = ad.creative_instance_id;
  expected_ad_content.creative_set_id = ad.creative_set_id;
  expected_ad_content.campaign_id = ad.campaign_id;
  expected_ad_content.advertiser_id = ad.advertiser_id;
  expected_ad_content.brand = kTitle;
  expected_ad_content.brand_info = kDescription;
  expected_ad_content.brand_display_url = ad.target_url.host();
  expected_ad_content.brand_url = ad.target_url;
  expected_ad_content.like_action_type = AdContentLikeActionType::kNeutral;
  expected_ad_content.confirmation_type = ConfirmationType::kViewed;

  EXPECT_EQ(expected_ad_content, ad_content);
}

}  // namespace ads
