/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_util.h"

#include "brave/components/brave_ads/core/internal/ad_units/test/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/test/creative_notification_ad_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

mojom::NotificationAdInfoPtr BuildNotificationAdMojom() {
  mojom::NotificationAdInfoPtr mojom_ad = mojom::NotificationAdInfo::New();
  mojom_ad->type = mojom::AdType::kNotificationAd;
  mojom_ad->placement_id = test::kPlacementId;
  mojom_ad->creative_instance_id = test::kCreativeInstanceId;
  mojom_ad->creative_set_id = test::kCreativeSetId;
  mojom_ad->campaign_id = test::kCampaignId;
  mojom_ad->advertiser_id = test::kAdvertiserId;
  mojom_ad->segment = test::kSegment;
  mojom_ad->title = "Test Ad Title";
  mojom_ad->body = "Test Ad Body";
  mojom_ad->target_url = GURL(test::kTargetUrl);
  return mojom_ad;
}

}  // namespace

class BraveAdsNotificationAdMojomUtilTest : public test::TestBase {};

TEST_F(BraveAdsNotificationAdMojomUtilTest, NotificationAdToMojom) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/false);
  const NotificationAdInfo ad =
      BuildNotificationAd(creative_ad, test::kPlacementId);

  // Act
  const mojom::NotificationAdInfoPtr mojom_ad = ToMojom(ad);

  // Assert
  EXPECT_EQ(mojom_ad, BuildNotificationAdMojom());
}

TEST_F(BraveAdsNotificationAdMojomUtilTest, InvalidNotificationAdToMojom) {
  // Arrange
  const NotificationAdInfo invalid_ad;

  // Act
  const mojom::NotificationAdInfoPtr mojom_ad = ToMojom(invalid_ad);

  // Assert
  EXPECT_FALSE(mojom_ad);
}

}  // namespace brave_ads
