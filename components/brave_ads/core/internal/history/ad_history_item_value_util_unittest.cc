/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/history/ad_history_item_value_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_builder_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

// The fields `ad_content`, `category_content`, and `created_at` in the legacy
// format have been renamed to `adContent`, `categoryContent`, and `createdAt`.
constexpr char kAdHistoryItemAsJson[] =
    R"JSON(
        {
          "adContent": {
            "adAction": "view",
            "adType": "ad_notification",
            "advertiserId": "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2",
            "brand": "Test Ad Title",
            "brandDisplayUrl": "brave.com",
            "brandInfo": "Test Ad Description",
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
        })JSON";

constexpr char kLegacyAdHistoryItemAsJson[] =
    R"JSON(
        {
          "ad_content": {
            "adAction": "view",
            "adType": "ad_notification",
            "advertiserId": "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2",
            "brand": "Test Ad Title",
            "brandDisplayUrl": "brave.com",
            "brandInfo": "Test Ad Description",
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
        })JSON";

}  // namespace

class BraveAdsAdHistoryItemValueUtilTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    AdvanceClockTo(
        test::TimeFromUTCString("Fri, 28 Sep 2012 17:45"));  // Hello Jaxson!!!
  }
};

TEST_F(BraveAdsAdHistoryItemValueUtilTest, AdHistoryItemFromValue) {
  // Arrange
  const base::Value::Dict dict =
      base::test::ParseJsonDict(kAdHistoryItemAsJson);

  // Act
  const AdHistoryItemInfo ad_history_item = AdHistoryItemFromValue(dict);

  // Assert
  EXPECT_EQ(ad_history_item,
            test::BuildAdHistoryItem(mojom::AdType::kNotificationAd,
                                     mojom::ConfirmationType::kViewedImpression,
                                     /*should_generate_random_uuids=*/false));
}

TEST_F(BraveAdsAdHistoryItemValueUtilTest, AdHistoryItemFromLegacyValue) {
  // Arrange
  const base::Value::Dict dict =
      base::test::ParseJsonDict(kLegacyAdHistoryItemAsJson);

  // Act
  const AdHistoryItemInfo ad_history_item = AdHistoryItemFromValue(dict);

  // Assert
  EXPECT_EQ(ad_history_item,
            test::BuildAdHistoryItem(mojom::AdType::kNotificationAd,
                                     mojom::ConfirmationType::kViewedImpression,
                                     /*should_generate_random_uuids=*/false));
}

TEST_F(BraveAdsAdHistoryItemValueUtilTest, AdHistoryItemToValue) {
  // Arrange
  const AdHistoryItemInfo ad_history_item =
      test::BuildAdHistoryItem(mojom::AdType::kNotificationAd,
                               mojom::ConfirmationType::kViewedImpression,
                               /*should_generate_random_uuids=*/false);

  // Act
  const base::Value::Dict dict = AdHistoryItemToValue(ad_history_item);

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(kAdHistoryItemAsJson), dict);
}

}  // namespace brave_ads
