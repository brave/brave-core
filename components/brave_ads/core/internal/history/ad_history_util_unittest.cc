/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/ad_history_util.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_feature.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_manager.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

AdHistoryItemInfo BuildAndAddHistoryItem() {
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  return AppendAdHistoryItem(ad, ConfirmationType::kViewedImpression, ad.title,
                             ad.body);
}

}  // namespace

class BraveAdsAdHistoryUtilTest : public test::TestBase {};

TEST_F(BraveAdsAdHistoryUtilTest, AppendAdHistoryItem) {
  // Act
  const AdHistoryItemInfo ad_history_item = BuildAndAddHistoryItem();

  // Assert
  const AdHistoryList expected_ad_history = {ad_history_item};
  EXPECT_THAT(expected_ad_history,
              ::testing::ElementsAreArray(AdHistoryManager::Get()));
}

TEST_F(BraveAdsAdHistoryUtilTest, PurgeHistoryOlderThanTimeWindow) {
  // Arrange
  BuildAndAddHistoryItem();

  AdvanceClockBy(kAdHistoryRetentionPeriod.Get() + base::Milliseconds(1));

  // Act
  const AdHistoryItemInfo ad_history_item = BuildAndAddHistoryItem();

  // Assert
  const AdHistoryList expected_ad_history = {ad_history_item};
  EXPECT_THAT(expected_ad_history,
              ::testing::ElementsAreArray(AdHistoryManager::Get()));
}

TEST_F(BraveAdsAdHistoryUtilTest, DoNotPurgeHistoryWithinTimeWindow) {
  // Arrange
  const AdHistoryItemInfo ad_history_item_1 = BuildAndAddHistoryItem();

  AdvanceClockBy(kAdHistoryRetentionPeriod.Get());

  // Act
  const AdHistoryItemInfo ad_history_item_2 = BuildAndAddHistoryItem();

  // Assert
  const AdHistoryList expected_ad_history = {ad_history_item_2,
                                             ad_history_item_1};
  EXPECT_THAT(expected_ad_history,
              ::testing::ElementsAreArray(AdHistoryManager::Get()));
}

}  // namespace brave_ads
