/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/history/history_item_value_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/history/history_item_util.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/history/history_item_info.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kJson[] =
    R"(
        [
          {
            "ad_content": {
              "adAction": "view",
              "adType": "ad_notification",
              "advertiserId": "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2",
              "brand": "Test Ad Title",
              "brandDisplayUrl": "brave.com",
              "brandInfo": "Test Ad Body",
              "brandUrl": "https://brave.com/",
              "campaignId": "84197fc8-830a-4a8e-8339-7a70c2bfa104",
              "creativeInstanceId": "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
              "creativeSetId": "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123",
              "flaggedAd": false,
              "likeAction": 0,
              "placementId": "9bac9ae4-693c-4569-9b3e-300e357780cf",
              "savedAd": false,
              "segment": "untargeted"
            },
            "category_content": {
              "category": "untargeted",
              "optAction": 0
            },
            "created_at": "12993327900000000"
          },
          {
            "ad_content": {
              "adAction": "view",
              "adType": "ad_notification",
              "advertiserId": "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2",
              "brand": "Test Ad Title",
              "brandDisplayUrl": "brave.com",
              "brandInfo": "Test Ad Body",
              "brandUrl": "https://brave.com/",
              "campaignId": "84197fc8-830a-4a8e-8339-7a70c2bfa104",
              "creativeInstanceId": "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
              "creativeSetId": "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123",
              "flaggedAd": false,
              "likeAction": 0,
              "placementId": "9bac9ae4-693c-4569-9b3e-300e357780cf",
              "savedAd": false,
              "segment": "untargeted"
            },
            "category_content": {
              "category": "untargeted",
              "optAction": 0
            },
            "created_at": "12993327900000000"
          }
        ])";

HistoryItemList BuildHistoryItems() {
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids=*/false);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad, kPlacementId);

  const HistoryItemInfo history_item =
      BuildHistoryItem(ad, ConfirmationType::kViewed, ad.title, ad.body);

  return {history_item, history_item};
}

}  // namespace

class BraveAdsHistoryItemValueUtilTest : public UnitTestBase {};

TEST_F(BraveAdsHistoryItemValueUtilTest, FromValue) {
  // Arrange
  AdvanceClockTo(TimeFromString("Fri, 28 Sep 2012 17:45",
                                /*is_local=*/false));  // Hello Jaxson!!!

  const base::Value::List list = base::test::ParseJsonList(kJson);

  // Act & Assert
  EXPECT_THAT(HistoryItemsFromValue(list),
              ::testing::ElementsAreArray(BuildHistoryItems()));
}

TEST_F(BraveAdsHistoryItemValueUtilTest, ToValue) {
  // Arrange
  AdvanceClockTo(TimeFromString("Fri, 28 Sep 2012 17:45", /*is_local=*/false));

  const HistoryItemList history_items = BuildHistoryItems();

  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonList(kJson),
            HistoryItemsToValue(history_items));
}

}  // namespace brave_ads
