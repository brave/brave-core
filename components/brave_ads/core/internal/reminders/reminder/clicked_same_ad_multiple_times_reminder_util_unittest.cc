/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/reminders/reminder/clicked_same_ad_multiple_times_reminder_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/ads_observer_mock.h"
#include "brave/components/brave_ads/core/internal/ads_observer_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_feature.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_util.h"
#include "brave/components/brave_ads/core/internal/reminders/reminders_feature.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kAdHistoryItemTitle[] = "title";
constexpr char kAdHistoryItemDescription[] = "description";

AdHistoryItemInfo AppendAdHistoryItem(
    const AdInfo& ad,
    const ConfirmationType confirmation_type) {
  return AppendAdHistoryItem(ad, confirmation_type, kAdHistoryItemTitle,
                             kAdHistoryItemDescription);
}

AdHistoryItemInfo AppendAdHistoryItems(
    const int count,
    const bool should_generate_random_uuids) {
  CHECK_GT(count, 0);

  AdHistoryItemInfo ad_history_item;

  AdInfo ad;

  for (int i = 0; i < count; ++i) {
    if (i == 0 || should_generate_random_uuids) {
      ad = test::BuildAd(AdType::kNotificationAd,
                         /*should_generate_random_uuids=*/true);
    }

    ad_history_item = AppendAdHistoryItem(ad, ConfirmationType::kClicked);
  }

  return ad_history_item;
}

}  // namespace

class BraveAdsClickedSameAdMultipleTimesReminderUtilTest
    : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    ads_observer_mock_ = test::AddAdsObserverMock();
  }

  raw_ptr<AdsObserverMock> ads_observer_mock_ = nullptr;
};

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       RemindUserOnDesktopOperatingSystems) {
  // Arrange
  const AdHistoryItemInfo ad_history_item =
      AppendAdHistoryItems(/*count=*/kRemindUserIfClickingTheSameAdAfter.Get(),
                           /*should_generate_random_uuids=*/false);

  // Act & Assert
  EXPECT_TRUE(DidUserClickTheSameAdMultipleTimes(ad_history_item));
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       DoNotRemindUserOnMobileOperatingSystems) {
  // Arrange
  test::MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  const AdHistoryItemInfo ad_history_item =
      AppendAdHistoryItems(/*count=*/kRemindUserIfClickingTheSameAdAfter.Get(),
                           /*should_generate_random_uuids=*/false);

  // Act & Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(ad_history_item));
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       RemindUserAfterClickingTheSameAdMultipleTimes) {
  // Arrange
  const AdHistoryItemInfo ad_history_item =
      AppendAdHistoryItems(/*count=*/kRemindUserIfClickingTheSameAdAfter.Get(),
                           /*should_generate_random_uuids=*/false);

  // Act & Assert
  EXPECT_TRUE(DidUserClickTheSameAdMultipleTimes(ad_history_item));
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       DoNotRemindUserIfTheyDidNotClickTheSameAdMultipleTimes) {
  // Arrange
  const AdHistoryItemInfo ad_history_item = AppendAdHistoryItems(
      /*count=*/kRemindUserIfClickingTheSameAdAfter.Get() - 1,
      /*should_generate_random_uuids=*/false);

  // Act & Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(ad_history_item));
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       RemindUserAfterOnceAgainClickingTheSameAdMultipleTimes) {
  // Arrange
  const AdHistoryItemInfo ad_history_item = AppendAdHistoryItems(
      /*count=*/kRemindUserIfClickingTheSameAdAfter.Get() * 2,
      /*should_generate_random_uuids=*/false);

  // Act & Assert
  EXPECT_TRUE(DidUserClickTheSameAdMultipleTimes(ad_history_item));
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       DoNotRemindUserIfTheyDidNotOnceAgainClickTheSameAdMultipleTimes) {
  // Arrange
  const AdHistoryItemInfo ad_history_item = AppendAdHistoryItems(
      /*count=*/(kRemindUserIfClickingTheSameAdAfter.Get() * 2) - 1,
      /*should_generate_random_uuids=*/false);

  // Act & Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(ad_history_item));
}

TEST_F(
    BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
    RemindUserAfterClickingTheSameAdMultipleTimesOnTheCuspOfExpiringHistory) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);

  AppendAdHistoryItem(ad, ConfirmationType::kClicked);
  AppendAdHistoryItem(ad, ConfirmationType::kClicked);

  AdvanceClockBy(kAdHistoryRetentionPeriod.Get() - base::Milliseconds(1));

  const AdHistoryItemInfo ad_history_item =
      AppendAdHistoryItem(ad, ConfirmationType::kClicked);

  // Act & Assert
  EXPECT_TRUE(DidUserClickTheSameAdMultipleTimes(ad_history_item));
}

TEST_F(
    BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
    DoNotRemindUserIfTheyDidNotClickTheSameAdMultipleTimesAfterTheHistoryHasExpired) {
  // Arrange
  AppendAdHistoryItems(
      /*count=*/kRemindUserIfClickingTheSameAdAfter.Get() - 1,
      /*should_generate_random_uuids=*/false);

  AdvanceClockBy(kAdHistoryRetentionPeriod.Get());

  const AdHistoryItemInfo ad_history_item = AppendAdHistoryItems(
      /*count=*/1, /*should_generate_random_uuids=*/false);

  // Act & Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(ad_history_item));
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       DoNotRemindTheUserAfterClickingDifferentAds) {
  // Arrange
  const AdHistoryItemInfo ad_history_item = AppendAdHistoryItems(
      /*count=*/kRemindUserIfClickingTheSameAdAfter.Get(),
      /*should_generate_random_uuids=*/true);

  // Act & Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(ad_history_item));
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       DoNotRemindTheUserForTheSameAdWithDifferentConfirmationTypes) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);

  AppendAdHistoryItem(ad, ConfirmationType::kServedImpression);
  AppendAdHistoryItem(ad, ConfirmationType::kViewedImpression);
  const AdHistoryItemInfo ad_history_item =
      AppendAdHistoryItem(ad, ConfirmationType::kClicked);

  // Act & Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(ad_history_item));
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       RemindUserTheyDoNotNeedToClickToEarnRewards) {
  // Act & Assert
  EXPECT_CALL(*ads_observer_mock_,
              OnRemindUser(mojom::ReminderType::kClickedSameAdMultipleTimes));
  RemindUserTheyDoNotNeedToClickToEarnRewards();
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       RemindUserMultipleTimesTheyDoNotNeedToClickToEarnRewards) {
  // Arrange
  RemindUserTheyDoNotNeedToClickToEarnRewards();

  // Act & Assert
  EXPECT_CALL(*ads_observer_mock_,
              OnRemindUser(mojom::ReminderType::kClickedSameAdMultipleTimes));
  RemindUserTheyDoNotNeedToClickToEarnRewards();
}

}  // namespace brave_ads
