/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/history_util.h"

#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/history_item_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/history/history_constants.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsHistoryUtilTest : public UnitTestBase {};

TEST_F(BraveAdsHistoryUtilTest, AddHistory) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  // Act
  const HistoryItemInfo history_item =
      AddHistory(ad, ConfirmationType::kViewed, ad.title, ad.body);

  // Assert
  const HistoryItemList expected_history = {history_item};

  const HistoryItemList& history =
      ClientStateManager::GetInstance().GetHistory();

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
}

TEST_F(BraveAdsHistoryUtilTest, PurgeHistoryOlderThanTimeWindow) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
  const NotificationAdInfo ad_1 = BuildNotificationAd(creative_ad_1);
  HistoryManager::GetInstance().Add(ad_1, ConfirmationType::kViewed);

  AdvanceClockBy(kHistoryTimeWindow + base::Milliseconds(1));

  // Act
  const CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
  const NotificationAdInfo ad_2 = BuildNotificationAd(creative_ad_2);
  const HistoryItemInfo history_item_2 =
      HistoryManager::GetInstance().Add(ad_2, ConfirmationType::kViewed);

  // Assert
  const HistoryItemList expected_history = {history_item_2};

  const HistoryItemList& history =
      ClientStateManager::GetInstance().GetHistory();

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
}

TEST_F(BraveAdsHistoryUtilTest, DoNotPurgeHistoryWithinTimeWindow) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
  const NotificationAdInfo ad_1 = BuildNotificationAd(creative_ad_1);
  const HistoryItemInfo history_item_1 =
      HistoryManager::GetInstance().Add(ad_1, ConfirmationType::kViewed);

  AdvanceClockBy(kHistoryTimeWindow);

  // Act
  const CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
  const NotificationAdInfo ad_2 = BuildNotificationAd(creative_ad_2);
  const HistoryItemInfo history_item_2 =
      HistoryManager::GetInstance().Add(ad_2, ConfirmationType::kViewed);

  // Assert
  const HistoryItemList expected_history = {history_item_2, history_item_1};

  const HistoryItemList& history =
      ClientStateManager::GetInstance().GetHistory();

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
}

}  // namespace brave_ads
