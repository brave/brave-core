/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/history/history_util.h"

#include "base/ranges/algorithm.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "bat/ads/internal/creatives/notification_ads/notification_ad_builder.h"
#include "bat/ads/internal/history/history_constants.h"
#include "bat/ads/internal/history/history_manager.h"
#include "bat/ads/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsHistoryUtilTest : public UnitTestBase {};

TEST_F(BatAdsHistoryUtilTest, AddHistory) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  // Act
  const HistoryItemInfo history_item =
      AddHistory(ad, ConfirmationType::kViewed, ad.title, ad.body);

  // Assert
  const HistoryItemList expected_history = {history_item};

  const HistoryItemList& history =
      ClientStateManager::GetInstance()->GetHistory();

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
}

TEST_F(BatAdsHistoryUtilTest, PurgeHistoryOlderThanTimeWindow) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAd();
  const NotificationAdInfo ad_1 = BuildNotificationAd(creative_ad_1);
  HistoryManager::GetInstance()->Add(ad_1, ConfirmationType::kViewed);

  AdvanceClockBy(kHistoryTimeWindow + base::Seconds(1));

  // Act
  const CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAd();
  const NotificationAdInfo ad_2 = BuildNotificationAd(creative_ad_2);
  const HistoryItemInfo history_item_2 =
      HistoryManager::GetInstance()->Add(ad_2, ConfirmationType::kViewed);

  // Assert
  const HistoryItemList expected_history = {history_item_2};

  const HistoryItemList& history =
      ClientStateManager::GetInstance()->GetHistory();

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
}

TEST_F(BatAdsHistoryUtilTest, DoNotPurgeHistoryWithinTimeWindow) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAd();
  const NotificationAdInfo ad_1 = BuildNotificationAd(creative_ad_1);
  const HistoryItemInfo history_item_1 =
      HistoryManager::GetInstance()->Add(ad_1, ConfirmationType::kViewed);

  AdvanceClockBy(kHistoryTimeWindow);

  // Act
  const CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAd();
  const NotificationAdInfo ad_2 = BuildNotificationAd(creative_ad_2);
  const HistoryItemInfo history_item_2 =
      HistoryManager::GetInstance()->Add(ad_2, ConfirmationType::kViewed);

  // Assert
  const HistoryItemList expected_history = {history_item_2, history_item_1};

  const HistoryItemList& history =
      ClientStateManager::GetInstance()->GetHistory();

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
}

}  // namespace ads
