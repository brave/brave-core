/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_history/ads_history.h"

#include <deque>

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/new_tab_page_ad_info.h"
#include "bat/ads/promoted_content_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsAdsHistoryTest : public UnitTestBase {
 protected:
  BatAdsAdsHistoryTest() = default;

  ~BatAdsAdsHistoryTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    AdvanceClock(base::TimeDelta::FromDays(history::kForDays));
  }
};

TEST_F(BatAdsAdsHistoryTest, AddAdNotificationToEmptyHistory) {
  // Arrange
  AdNotificationInfo ad;

  // Act
  history::AddAdNotification(ad, ConfirmationType::kViewed);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(1UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, AddAdNotificationsToHistory) {
  // Arrange
  AdNotificationInfo ad;

  // Act
  history::AddAdNotification(ad, ConfirmationType::kViewed);
  history::AddAdNotification(ad, ConfirmationType::kClicked);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(2UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, AddNewTabPageAdToEmptyHistory) {
  // Arrange
  NewTabPageAdInfo ad;

  // Act
  history::AddNewTabPageAd(ad, ConfirmationType::kViewed);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(1UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, AddNewTabPageAdsToHistory) {
  // Arrange
  NewTabPageAdInfo ad;

  // Act
  history::AddNewTabPageAd(ad, ConfirmationType::kViewed);
  history::AddNewTabPageAd(ad, ConfirmationType::kClicked);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(2UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, AddPromotedContentAdToEmptyHistory) {
  // Arrange
  PromotedContentAdInfo ad;

  // Act
  history::AddPromotedContentAd(ad, ConfirmationType::kViewed);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(1UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, AddPromotedContentAdsToHistory) {
  // Arrange
  PromotedContentAdInfo ad;

  // Act
  history::AddPromotedContentAd(ad, ConfirmationType::kViewed);
  history::AddPromotedContentAd(ad, ConfirmationType::kClicked);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(2UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, AddInlineContentAdToEmptyHistory) {
  // Arrange
  InlineContentAdInfo ad;

  // Act
  history::AddInlineContentAd(ad, ConfirmationType::kViewed);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(1UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, AddInlineContentAdsToHistory) {
  // Arrange
  InlineContentAdInfo ad;

  // Act
  history::AddInlineContentAd(ad, ConfirmationType::kViewed);
  history::AddInlineContentAd(ad, ConfirmationType::kClicked);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(2UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, AddMultipleAdTypesToHistory) {
  // Arrange

  // Act
  AdNotificationInfo ad_notification;
  history::AddAdNotification(ad_notification, ConfirmationType::kViewed);

  NewTabPageAdInfo new_tab_page_ad;
  history::AddNewTabPageAd(new_tab_page_ad, ConfirmationType::kViewed);

  PromotedContentAdInfo promoted_content_ad;
  history::AddPromotedContentAd(promoted_content_ad, ConfirmationType::kViewed);

  InlineContentAdInfo inline_content_ad;
  history::AddInlineContentAd(inline_content_ad, ConfirmationType::kViewed);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(4UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, PurgedHistoryEntriesAfter7Days) {
  // Arrange
  NewTabPageAdInfo new_tab_page_ad;
  history::AddNewTabPageAd(new_tab_page_ad, ConfirmationType::kViewed);

  AdvanceClock(base::TimeDelta::FromDays(7) + base::TimeDelta::FromSeconds(1));

  // Act
  PromotedContentAdInfo promoted_content_ad;
  history::AddPromotedContentAd(promoted_content_ad, ConfirmationType::kViewed);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(1UL, history.size());
}

}  // namespace ads
