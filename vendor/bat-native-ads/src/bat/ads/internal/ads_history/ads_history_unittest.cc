/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_history/ads_history.h"

#include <deque>

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/new_tab_page_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsAdsHistoryTest : public UnitTestBase {
 protected:
  BatAdsAdsHistoryTest() = default;

  ~BatAdsAdsHistoryTest() override = default;
};

TEST_F(BatAdsAdsHistoryTest,
    AddAdNotificationToEmptyHistory) {
  // Arrange
  AdNotificationInfo ad;

  // Act
  history::AddAdNotification(ad, ConfirmationType::kViewed);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(1UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest,
    AddAdNotificationsToHistory) {
  // Arrange
  AdNotificationInfo ad;

  // Act
  history::AddAdNotification(ad, ConfirmationType::kViewed);
  history::AddAdNotification(ad, ConfirmationType::kClicked);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(2UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest,
    HistoryRespectsMaximumSizeForAdNotifications) {
  // Arrange
  AdNotificationInfo ad;

  // Act
  for (size_t i = 0; i < history::kMaximumEntries + 1; i++) {
    history::AddAdNotification(ad, ConfirmationType::kViewed);
  }

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(history::kMaximumEntries, history.size());
}

TEST_F(BatAdsAdsHistoryTest,
    AddNewTabPageAdToEmptyHistory) {
  // Arrange
  NewTabPageAdInfo ad;

  // Act
  history::AddNewTabPageAd(ad, ConfirmationType::kViewed);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(1UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest,
    AddNewTabPageAdsToHistory) {
  // Arrange
  NewTabPageAdInfo ad;

  // Act
  history::AddNewTabPageAd(ad, ConfirmationType::kViewed);
  history::AddNewTabPageAd(ad, ConfirmationType::kClicked);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(2UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest,
    HistoryRespectsMaximumSizeForNewTabPageAds) {
  // Arrange
  NewTabPageAdInfo ad;

  // Act
  for (size_t i = 0; i < history::kMaximumEntries + 1; i++) {
    history::AddNewTabPageAd(ad, ConfirmationType::kViewed);
  }

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(history::kMaximumEntries, history.size());
}

TEST_F(BatAdsAdsHistoryTest,
    MaximumHistoryEntries) {
  // Arrange

  // Act

  // Assert
  ASSERT_EQ(840UL, history::kMaximumEntries);
}

}  // namespace ads
