/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/ad_history_builder_util.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdHistoryBuilderUtilTest : public test::TestBase {};

TEST_F(BraveAdsAdHistoryBuilderUtilTest, BuildAdHistoryItem) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  // Act
  const AdHistoryItemInfo ad_history_item = BuildAdHistoryItem(
      ad, ConfirmationType::kViewedImpression, ad.title, ad.body);

  // Assert
  EXPECT_THAT(ad_history_item,
              ::testing::FieldsAre(
                  /*created_at*/ test::Now(), ad.type,
                  ConfirmationType::kViewedImpression, ad.placement_id,
                  ad.creative_instance_id, ad.creative_set_id, ad.campaign_id,
                  ad.advertiser_id, ad.segment, ad.title,
                  /*description*/ ad.body, ad.target_url));
}

}  // namespace brave_ads
