/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/history_item_value_util.h"

#include "base/ranges/algorithm.h"
#include "base/test/values_test_util.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "bat/ads/internal/creatives/notification_ads/notification_ad_builder.h"
#include "bat/ads/internal/history/history_item_util.h"
#include "bat/ads/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kPlacementId[] = "f0948316-df6f-4e31-814d-d0b5f2a1f28c";

constexpr char kJson[] =
    R"([{"ad_content":{"adAction":"view","adType":"ad_notification","advertiserId":"5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2","brand":"Test Ad Title","brandDisplayUrl":"brave.com","brandInfo":"Test Ad Body","brandUrl":"https://brave.com/","campaignId":"84197fc8-830a-4a8e-8339-7a70c2bfa104","creativeInstanceId":"3519f52c-46a4-4c48-9c2b-c264c0067f04","creativeSetId":"c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123","flaggedAd":false,"likeAction":0,"placementId":"f0948316-df6f-4e31-814d-d0b5f2a1f28c","savedAd":false},"category_content":{"category":"untargeted","optAction":0},"timestamp_in_seconds":"1679443200"},{"ad_content":{"adAction":"view","adType":"ad_notification","advertiserId":"5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2","brand":"Test Ad Title","brandDisplayUrl":"brave.com","brandInfo":"Test Ad Body","brandUrl":"https://brave.com/","campaignId":"84197fc8-830a-4a8e-8339-7a70c2bfa104","creativeInstanceId":"3519f52c-46a4-4c48-9c2b-c264c0067f04","creativeSetId":"c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123","flaggedAd":false,"likeAction":0,"placementId":"f0948316-df6f-4e31-814d-d0b5f2a1f28c","savedAd":false},"category_content":{"category":"untargeted","optAction":0},"timestamp_in_seconds":"1679443200"}])";

HistoryItemList BuildHistoryItems() {
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ false);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad, kPlacementId);

  const HistoryItemInfo history_item =
      BuildHistoryItem(ad, ConfirmationType::kViewed, ad.title, ad.body);

  return {history_item, history_item};
}

}  // namespace

class BatAdsHistoryItemValueUtilTest : public UnitTestBase {};

TEST_F(BatAdsHistoryItemValueUtilTest, FromValue) {
  // Arrange
  AdvanceClockTo(TimeFromString("22 March 2023", /*is_local*/ false));

  const base::Value value = base::test::ParseJson(kJson);
  const base::Value::List* const list = value.GetIfList();
  ASSERT_TRUE(list);

  // Act
  const HistoryItemList history_items = HistoryItemsFromValue(*list);

  // Assert
  EXPECT_TRUE(base::ranges::equal(BuildHistoryItems(), history_items));
}

TEST_F(BatAdsHistoryItemValueUtilTest, ToValue) {
  // Arrange
  AdvanceClockTo(TimeFromString("22 March 2023", /*is_local*/ false));

  const HistoryItemList history_items = BuildHistoryItems();

  // Act
  const base::Value::List value = HistoryItemsToValue(history_items);

  // Assert
  const base::Value expected_value = base::test::ParseJson(kJson);
  EXPECT_EQ(expected_value, value);
}

}  // namespace ads
