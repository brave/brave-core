/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_value_util.h"

#include "base/containers/circular_deque.h"
#include "base/ranges/algorithm.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kJson[] =
    R"(
        {
          "advertiser_id": "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2",
          "body": "Test Ad Body",
          "campaign_id": "84197fc8-830a-4a8e-8339-7a70c2bfa104",
          "creative_instance_id": "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
          "creative_set_id": "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123",
          "segment": "untargeted",
          "target_url": "https://brave.com/",
          "title": "Test Ad Title",
          "type": "ad_notification",
          "uuid": "9bac9ae4-693c-4569-9b3e-300e357780cf"
        })";

constexpr char kListJson[] =
    R"(
        [
          {
            "advertiser_id": "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2",
            "body": "Test Ad Body",
            "campaign_id": "84197fc8-830a-4a8e-8339-7a70c2bfa104",
            "creative_instance_id": "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
            "creative_set_id": "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123",
            "segment": "untargeted",
            "target_url": "https://brave.com/",
            "title": "Test Ad Title",
            "type": "ad_notification",
            "uuid": "9bac9ae4-693c-4569-9b3e-300e357780cf"
          },
          {
            "advertiser_id": "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2",
            "body": "Test Ad Body",
            "campaign_id": "84197fc8-830a-4a8e-8339-7a70c2bfa104",
            "creative_instance_id": "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
            "creative_set_id": "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123",
            "segment": "untargeted",
            "target_url": "https://brave.com/",
            "title": "Test Ad Title",
            "type": "ad_notification",
            "uuid": "9bac9ae4-693c-4569-9b3e-300e357780cf"
          }
        ])";

}  // namespace

class BraveAdsNotificationAdValueUtilTest : public UnitTestBase {};

TEST_F(BraveAdsNotificationAdValueUtilTest, ToValue) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/false);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad, kPlacementId);

  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(kJson), NotificationAdToValue(ad));
}

TEST_F(BraveAdsNotificationAdValueUtilTest, ToListValue) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/false);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad, kPlacementId);

  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonList(kListJson),
            NotificationAdsToValue({ad, ad}));
}

TEST_F(BraveAdsNotificationAdValueUtilTest, FromValue) {
  // Arrange
  const base::Value::Dict dict = base::test::ParseJsonDict(kJson);

  // Act & Assert
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/false);
  const NotificationAdInfo expected_ad =
      BuildNotificationAd(creative_ad, kPlacementId);
  EXPECT_EQ(expected_ad, NotificationAdFromValue(dict));
}

TEST_F(BraveAdsNotificationAdValueUtilTest, FromListValue) {
  // Arrange
  const base::Value::List list = base::test::ParseJsonList(kListJson);

  // Act & Assert
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/false);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad, kPlacementId);
  const base::circular_deque<NotificationAdInfo> expected_ads = {ad, ad};
  EXPECT_TRUE(
      base::ranges::equal(expected_ads, NotificationAdsFromValue(list)));
}

}  // namespace brave_ads
