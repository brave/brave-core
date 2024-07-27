/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/history/ad_history_value_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_builder_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kAdHistoryAsJson[] =
    R"(
        [
          {
            "adContent": {
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
            "categoryContent": {
              "category": "untargeted",
              "optAction": 0
            },
            "createdAt": "12993327900000000"
          },
          {
            "adContent": {
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
            "categoryContent": {
              "category": "untargeted",
              "optAction": 0
            },
            "createdAt": "12993327900000000"
          }
        ])";

constexpr char kLegacyAdHistoryAsJson[] =
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
            "createdAt": "12993327900000000"
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
            "createdAt": "12993327900000000"
          }
        ])";

AdHistoryList BuildAdHistory() {
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/false);
  const NotificationAdInfo ad =
      BuildNotificationAd(creative_ad, test::kPlacementId);

  const AdHistoryItemInfo ad_history_item = BuildAdHistoryItem(
      ad, ConfirmationType::kViewedImpression, ad.title, ad.body);

  return {ad_history_item, ad_history_item};
}

}  // namespace

class BraveAdsAdHistoryValueUtilTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    AdvanceClockTo(
        test::TimeFromUTCString("Fri, 28 Sep 2012 17:45"));  // Hello Jaxson!!!
  }
};

TEST_F(BraveAdsAdHistoryValueUtilTest, AdHistoryFromValue) {
  // Arrange
  const base::Value::List list = base::test::ParseJsonList(kAdHistoryAsJson);

  // Act
  const AdHistoryList ad_history = AdHistoryFromValue(list);

  // Assert
  EXPECT_THAT(ad_history, ::testing::ElementsAreArray(BuildAdHistory()));
}

TEST_F(BraveAdsAdHistoryValueUtilTest, LegacyAdHistoryFromValue) {
  // Arrange
  const base::Value::List list =
      base::test::ParseJsonList(kLegacyAdHistoryAsJson);

  // Act
  const AdHistoryList ad_history = AdHistoryFromValue(list);

  // Assert
  EXPECT_THAT(ad_history, ::testing::ElementsAreArray(BuildAdHistory()));
}

TEST_F(BraveAdsAdHistoryValueUtilTest, AdHistoryToValue) {
  // Arrange
  const AdHistoryList ad_history = BuildAdHistory();

  // Act
  const base::Value::List list = AdHistoryToValue(ad_history);

  // Assert
  EXPECT_EQ(base::test::ParseJsonList(kAdHistoryAsJson), list);
}

}  // namespace brave_ads
