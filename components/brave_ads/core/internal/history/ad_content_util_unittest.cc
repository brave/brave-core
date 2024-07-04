/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/ad_content_util.h"

#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kTitle[] = "title";
constexpr char kDescription[] = "description";

}  // namespace

class BraveAdsAdContentUtilTest : public UnitTestBase {};

TEST_F(BraveAdsAdContentUtilTest, Build) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  // Act
  const AdContentInfo ad_content = BuildAdContent(
      ad, ConfirmationType::kViewedImpression, kTitle, kDescription);

  // Assert
  EXPECT_THAT(
      ad_content,
      ::testing::FieldsAre(ad.type, ad.placement_id, ad.creative_instance_id,
                           ad.creative_set_id, ad.campaign_id, ad.advertiser_id,
                           kSegment, kTitle, kDescription, ad.target_url.host(),
                           ad.target_url, mojom::UserReactionType::kNeutral,
                           ConfirmationType::kViewedImpression,
                           /*is_saved*/ false, /*is_flagged*/ false));
}

}  // namespace brave_ads
