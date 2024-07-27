/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_value_util.h"

#include "base/containers/circular_deque.h"
#include "base/ranges/algorithm.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kNotificationAdAsJson[] =
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

constexpr char kNotificationAdsAsJson[] =
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

class BraveAdsNotificationAdValueUtilTest : public test::TestBase {};

TEST_F(BraveAdsNotificationAdValueUtilTest, NotificationAdToValue) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  const NotificationAdInfo ad =
      BuildNotificationAd(creative_ad, test::kPlacementId);

  // Act
  const base::Value::Dict dict = NotificationAdToValue(ad);

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(kNotificationAdAsJson), dict);
}

TEST_F(BraveAdsNotificationAdValueUtilTest, NotificationAdsToValue) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  const NotificationAdInfo ad =
      BuildNotificationAd(creative_ad, test::kPlacementId);

  // Act
  const base::Value::List list = NotificationAdsToValue({ad, ad});

  // Assert
  EXPECT_EQ(base::test::ParseJsonList(kNotificationAdsAsJson), list);
}

TEST_F(BraveAdsNotificationAdValueUtilTest, NotificationAdFromValue) {
  // Arrange
  const base::Value::Dict dict =
      base::test::ParseJsonDict(kNotificationAdAsJson);

  // Act
  const NotificationAdInfo ad = NotificationAdFromValue(dict);

  // Assert
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  EXPECT_EQ(BuildNotificationAd(creative_ad, test::kPlacementId), ad);
}

TEST_F(BraveAdsNotificationAdValueUtilTest, NotificationAdsFromValue) {
  // Arrange
  const base::Value::List list =
      base::test::ParseJsonList(kNotificationAdsAsJson);

  // Act
  const base::circular_deque<NotificationAdInfo> ads =
      NotificationAdsFromValue(list);

  // Assert
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  const NotificationAdInfo ad =
      BuildNotificationAd(creative_ad, test::kPlacementId);
  const base::circular_deque<NotificationAdInfo> expected_ads = {ad, ad};
  EXPECT_TRUE(base::ranges::equal(expected_ads, ads));
}

}  // namespace brave_ads
