/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/notification_ad_value_util.h"

#include "base/containers/circular_deque.h"
#include "base/ranges/algorithm.h"
#include "base/test/values_test_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "bat/ads/internal/creatives/notification_ads/notification_ad_builder.h"
#include "bat/ads/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kPlacementId[] = "f0948316-df6f-4e31-814d-d0b5f2a1f28c";

constexpr char kJson[] =
    R"({"advertiser_id":"5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2","body":"Test Ad Body","campaign_id":"84197fc8-830a-4a8e-8339-7a70c2bfa104","creative_instance_id":"3519f52c-46a4-4c48-9c2b-c264c0067f04","creative_set_id":"c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123","segment":"untargeted","target_url":"https://brave.com/","title":"Test Ad Title","type":"ad_notification","uuid":"f0948316-df6f-4e31-814d-d0b5f2a1f28c"})";
constexpr char kListJson[] =
    R"([{"advertiser_id":"5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2","body":"Test Ad Body","campaign_id":"84197fc8-830a-4a8e-8339-7a70c2bfa104","creative_instance_id":"3519f52c-46a4-4c48-9c2b-c264c0067f04","creative_set_id":"c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123","segment":"untargeted","target_url":"https://brave.com/","title":"Test Ad Title","type":"ad_notification","uuid":"f0948316-df6f-4e31-814d-d0b5f2a1f28c"},{"advertiser_id":"5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2","body":"Test Ad Body","campaign_id":"84197fc8-830a-4a8e-8339-7a70c2bfa104","creative_instance_id":"3519f52c-46a4-4c48-9c2b-c264c0067f04","creative_set_id":"c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123","segment":"untargeted","target_url":"https://brave.com/","title":"Test Ad Title","type":"ad_notification","uuid":"f0948316-df6f-4e31-814d-d0b5f2a1f28c"}])";

}  // namespace

class BatAdsNotificationAdValueUtilTest : public UnitTestBase {};

TEST_F(BatAdsNotificationAdValueUtilTest, ToValue) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ false);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad, kPlacementId);

  // Act
  const base::Value::Dict value = NotificationAdToValue(ad);

  // Assert
  const base::Value expected_value = base::test::ParseJson(kJson);
  EXPECT_EQ(expected_value, value);
}

TEST_F(BatAdsNotificationAdValueUtilTest, ToListValue) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ false);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad, kPlacementId);

  // Act
  const base::Value::List value = NotificationAdsToValue({ad, ad});

  // Assert
  const base::Value expected_value = base::test::ParseJson(kListJson);
  EXPECT_EQ(expected_value, value);
}

TEST_F(BatAdsNotificationAdValueUtilTest, FromValue) {
  // Arrange
  const base::Value value = base::test::ParseJson(kJson);
  const base::Value::Dict* const dict = value.GetIfDict();
  ASSERT_TRUE(dict);

  // Act
  const NotificationAdInfo ad = NotificationAdFromValue(*dict);

  // Assert
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ false);
  const NotificationAdInfo expected_ad =
      BuildNotificationAd(creative_ad, kPlacementId);
  EXPECT_EQ(expected_ad, ad);
}

TEST_F(BatAdsNotificationAdValueUtilTest, FromListValue) {
  // Arrange
  const base::Value value = base::test::ParseJson(kListJson);
  const base::Value::List* const list = value.GetIfList();
  ASSERT_TRUE(list);

  // Act
  const base::circular_deque<NotificationAdInfo> ads =
      NotificationAdsFromValue(*list);

  // Assert
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ false);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad, kPlacementId);
  const base::circular_deque<NotificationAdInfo> expected_ads = {ad, ad};
  EXPECT_TRUE(base::ranges::equal(expected_ads, ads));
}

}  // namespace ads
