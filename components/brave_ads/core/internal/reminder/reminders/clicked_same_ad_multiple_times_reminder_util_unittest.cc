/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/reminder/reminders/clicked_same_ad_multiple_times_reminder_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/history/history_feature.h"
#include "brave/components/brave_ads/core/internal/history/history_util.h"
#include "brave/components/brave_ads/core/internal/reminder/reminder_feature.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/public/history/history_item_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kHistoryTitle[] = "title";
constexpr char kHistoryDescription[] = "description";

HistoryItemInfo AddHistory(const AdInfo& ad,
                           const ConfirmationType confirmation_type) {
  return AddHistory(ad, confirmation_type, kHistoryTitle, kHistoryDescription);
}

HistoryItemInfo AddHistory(
    const int count,
    const bool should_use_random_creative_instance_uuid) {
  CHECK_GT(count, 0);

  HistoryItemInfo history_item;

  AdInfo ad;
  for (int i = 0; i < count; ++i) {
    if (i == 0 || should_use_random_creative_instance_uuid) {
      ad = test::BuildAd(AdType::kNotificationAd,
                         /*should_use_random_uuids=*/true);
      CHECK(ad.IsValid());
    }

    history_item = AddHistory(ad, ConfirmationType::kClicked);
  }

  return history_item;
}

}  // namespace

class BraveAdsClickedSameAdMultipleTimesReminderUtilTest : public UnitTestBase {
};

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       RemindUserOnDesktopOperatingSystems) {
  // Arrange
  const HistoryItemInfo history_item =
      AddHistory(/*count=*/kRemindUserIfClickingTheSameAdAfter.Get(),
                 /*should_use_random_creative_instance_uuid=*/false);

  // Assert
  EXPECT_TRUE(DidUserClickTheSameAdMultipleTimes(history_item));
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       DoNotRemindUserOnMobileOperatingSystems) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  const HistoryItemInfo history_item =
      AddHistory(/*count=*/3,
                 /*should_use_random_creative_instance_uuid=*/false);

  // Act & Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(history_item));
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       RemindUserAfterClickingTheSameAdMultipleTimes) {
  // Arrange
  const HistoryItemInfo history_item =
      AddHistory(/*count=*/kRemindUserIfClickingTheSameAdAfter.Get(),
                 /*should_use_random_creative_instance_uuid=*/false);

  // Act & Assert
  EXPECT_TRUE(DidUserClickTheSameAdMultipleTimes(history_item));
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       DoNotRemindUserIfTheyDidNotClickTheSameAdMultipleTimes) {
  // Arrange
  const HistoryItemInfo history_item = AddHistory(
      /*count=*/kRemindUserIfClickingTheSameAdAfter.Get() - 1,
      /*should_use_random_creative_instance_uuid=*/false);

  // Act & Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(history_item));
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       RemindUserAfterOnceAgainClickingTheSameAdMultipleTimes) {
  // Arrange
  const HistoryItemInfo history_item = AddHistory(
      /*count=*/kRemindUserIfClickingTheSameAdAfter.Get() * 2,
      /*should_use_random_creative_instance_uuid=*/false);

  // Act & Assert
  EXPECT_TRUE(DidUserClickTheSameAdMultipleTimes(history_item));
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       DoNotRemindUserIfTheyDidNotOnceAgainClickTheSameAdMultipleTimes) {
  // Arrange
  const HistoryItemInfo history_item = AddHistory(
      /*count=*/(kRemindUserIfClickingTheSameAdAfter.Get() * 2) - 1,
      /*should_use_random_creative_instance_uuid=*/false);

  // Act & Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(history_item));
}

TEST_F(
    BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
    RemindUserAfterClickingTheSameAdMultipleTimesOnTheCuspOfExpiringHistory) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);

  AddHistory(ad, ConfirmationType::kClicked);
  AddHistory(ad, ConfirmationType::kClicked);

  AdvanceClockBy(kHistoryTimeWindow.Get() - base::Milliseconds(1));

  // Act
  const HistoryItemInfo history_item =
      AddHistory(ad, ConfirmationType::kClicked);

  // Assert
  EXPECT_TRUE(DidUserClickTheSameAdMultipleTimes(history_item));
}

TEST_F(
    BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
    DoNotRemindUserIfTheyDidNotClickTheSameAdMultipleTimesAfterTheHistoryHasExpired) {
  // Arrange
  AddHistory(
      /*count=*/kRemindUserIfClickingTheSameAdAfter.Get() - 1,
      /*should_use_random_creative_instance_uuid=*/false);

  AdvanceClockBy(kHistoryTimeWindow.Get());

  // Act
  const HistoryItemInfo history_item = AddHistory(
      /*count=*/1, /*should_use_random_creative_instance_uuid=*/false);

  // Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(history_item));
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       DoNotRemindTheUserAfterClickingDifferentAds) {
  // Arrange
  const HistoryItemInfo history_item = AddHistory(
      /*count=*/kRemindUserIfClickingTheSameAdAfter.Get(),
      /*should_use_random_creative_instance_uuid=*/true);

  // Act & Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(history_item));
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       DoNotRemindTheUserForTheSameAdWithDifferentConfirmationTypes) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);

  AddHistory(ad, ConfirmationType::kServed);
  AddHistory(ad, ConfirmationType::kViewed);
  const HistoryItemInfo history_item =
      AddHistory(ad, ConfirmationType::kClicked);

  // Act & Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(history_item));
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       RemindUserTheyDoNotNeedToClickToEarnRewards) {
  // Act & Assert
  EXPECT_CALL(ads_client_mock_,
              ShowReminder(mojom::ReminderType::kClickedSameAdMultipleTimes));
  RemindUserTheyDoNotNeedToClickToEarnRewards();
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       RemindUserMultipleTimesTheyDoNotNeedToClickToEarnRewards) {
  // Arrange
  RemindUserTheyDoNotNeedToClickToEarnRewards();

  // Act & Assert
  EXPECT_CALL(ads_client_mock_,
              ShowReminder(mojom::ReminderType::kClickedSameAdMultipleTimes));
  RemindUserTheyDoNotNeedToClickToEarnRewards();
}

}  // namespace brave_ads
