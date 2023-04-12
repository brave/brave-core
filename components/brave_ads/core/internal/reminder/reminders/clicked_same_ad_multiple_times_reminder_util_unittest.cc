/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/reminder/reminders/clicked_same_ad_multiple_times_reminder_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/history_item_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/history/history_constants.h"
#include "brave/components/brave_ads/core/internal/history/history_util.h"
#include "brave/components/brave_ads/core/internal/reminder/reminder_features.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

namespace {

constexpr char kHistoryTitle[] = "title";
constexpr char kHistoryDescription[] = "description";

HistoryItemInfo AddHistory(const AdInfo& ad,
                           const ConfirmationType confirmation_type) {
  return AddHistory(ad, confirmation_type, kHistoryTitle, kHistoryDescription);
}

HistoryItemInfo AddHistory(
    const size_t count,
    const bool should_use_random_creative_instance_guid) {
  CHECK_GT(count, 0U);

  HistoryItemInfo history_item;

  AdInfo ad;
  for (size_t i = 0; i < count; i++) {
    if (i == 0 || should_use_random_creative_instance_guid) {
      ad = BuildAd(AdType::kNotificationAd, /*should_use_random_guids*/ true);
      CHECK(ad.IsValid());
    }

    history_item = AddHistory(ad, ConfirmationType::kClicked);
  }

  return history_item;
}

}  // namespace

class BatAdsClickedSameAdMultipleTimesReminderUtilTest : public UnitTestBase {};

TEST_F(BatAdsClickedSameAdMultipleTimesReminderUtilTest,
       RemindUserOnDesktopOperatingSystems) {
  // Arrange
  const HistoryItemInfo history_item =
      AddHistory(/*count*/ features::GetRemindUserIfClickingTheSameAdAfter(),
                 /*should_use_random_creative_instance_guid*/ false);

  // Assert
  EXPECT_TRUE(DidUserClickTheSameAdMultipleTimes(history_item));
}

TEST_F(BatAdsClickedSameAdMultipleTimesReminderUtilTest,
       DoNotRemindUserOnMobileOperatingSystems) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  const HistoryItemInfo history_item =
      AddHistory(/*count*/ 3,
                 /*should_use_random_creative_instance_guid*/ false);

  // Act

  // Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(history_item));
}

TEST_F(BatAdsClickedSameAdMultipleTimesReminderUtilTest,
       RemindUserAfterClickingTheSameAdMultipleTimes) {
  // Arrange
  const HistoryItemInfo history_item =
      AddHistory(/*count*/ features::GetRemindUserIfClickingTheSameAdAfter(),
                 /*should_use_random_creative_instance_guid*/ false);

  // Act

  // Assert
  EXPECT_TRUE(DidUserClickTheSameAdMultipleTimes(history_item));
}

TEST_F(BatAdsClickedSameAdMultipleTimesReminderUtilTest,
       DoNotRemindUserIfTheyDidNotClickTheSameAdMultipleTimes) {
  // Arrange
  const HistoryItemInfo history_item = AddHistory(
      /*count*/ features::GetRemindUserIfClickingTheSameAdAfter() - 1,
      /*should_use_random_creative_instance_guid*/ false);

  // Act

  // Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(history_item));
}

TEST_F(BatAdsClickedSameAdMultipleTimesReminderUtilTest,
       RemindUserAfterOnceAgainClickingTheSameAdMultipleTimes) {
  // Arrange
  const HistoryItemInfo history_item = AddHistory(
      /*count*/ features::GetRemindUserIfClickingTheSameAdAfter() * 2,
      /*should_use_random_creative_instance_guid*/ false);

  // Act

  // Assert
  EXPECT_TRUE(DidUserClickTheSameAdMultipleTimes(history_item));
}

TEST_F(BatAdsClickedSameAdMultipleTimesReminderUtilTest,
       DoNotRemindUserIfTheyDidNotOnceAgainClickTheSameAdMultipleTimes) {
  // Arrange
  const HistoryItemInfo history_item = AddHistory(
      /*count*/ (features::GetRemindUserIfClickingTheSameAdAfter() * 2) - 1,
      /*should_use_random_creative_instance_guid*/ false);

  // Act

  // Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(history_item));
}

TEST_F(
    BatAdsClickedSameAdMultipleTimesReminderUtilTest,
    RemindUserAfterClickingTheSameAdMultipleTimesOnTheCuspOfExpiringHistory) {
  // Arrange
  const AdInfo ad =
      BuildAd(AdType::kNotificationAd, /*should_use_random_guids*/ false);

  AddHistory(ad, ConfirmationType::kClicked);
  AddHistory(ad, ConfirmationType::kClicked);

  AdvanceClockBy(kHistoryTimeWindow - base::Nanoseconds(1));

  // Act
  const HistoryItemInfo history_item =
      AddHistory(ad, ConfirmationType::kClicked);

  // Assert
  EXPECT_TRUE(DidUserClickTheSameAdMultipleTimes(history_item));
}

TEST_F(
    BatAdsClickedSameAdMultipleTimesReminderUtilTest,
    DoNotRemindUserIfTheyDidNotClickTheSameAdMultipleTimesAfterTheHistoryHasExpired) {
  // Arrange
  AddHistory(
      /*count*/ features::GetRemindUserIfClickingTheSameAdAfter() - 1,
      /*should_use_random_creative_instance_guid*/ false);

  AdvanceClockBy(kHistoryTimeWindow);

  // Act
  const HistoryItemInfo history_item = AddHistory(
      /*count*/ 1, /*should_use_random_creative_instance_guid*/ false);

  // Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(history_item));
}

TEST_F(BatAdsClickedSameAdMultipleTimesReminderUtilTest,
       DoNotRemindTheUserAfterClickingDifferentAds) {
  // Arrange
  const HistoryItemInfo history_item = AddHistory(
      /*count*/ features::GetRemindUserIfClickingTheSameAdAfter(),
      /*should_use_random_creative_instance_guid*/ true);

  // Act

  // Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(history_item));
}

TEST_F(BatAdsClickedSameAdMultipleTimesReminderUtilTest,
       DoNotRemindTheUserForTheSameAdWithDifferentConfirmationTypes) {
  // Arrange
  const AdInfo ad =
      BuildAd(AdType::kNotificationAd, /*should_use_random_guids*/ false);

  AddHistory(ad, ConfirmationType::kServed);
  AddHistory(ad, ConfirmationType::kViewed);
  const HistoryItemInfo history_item =
      AddHistory(ad, ConfirmationType::kClicked);

  // Act

  // Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(history_item));
}

TEST_F(BatAdsClickedSameAdMultipleTimesReminderUtilTest,
       RemindUserTheyDoNotNeedToClickToEarnRewards) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_,
              ShowReminder(mojom::ReminderType::kClickedSameAdMultipleTimes));

  // Act
  RemindUserTheyDoNotNeedToClickToEarnRewards();

  // Assert
}

TEST_F(BatAdsClickedSameAdMultipleTimesReminderUtilTest,
       RemindUserMultipleTimesTheyDoNotNeedToClickToEarnRewards) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_,
              ShowReminder(mojom::ReminderType::kClickedSameAdMultipleTimes))
      .Times(2);

  RemindUserTheyDoNotNeedToClickToEarnRewards();

  // Act
  RemindUserTheyDoNotNeedToClickToEarnRewards();

  // Assert
}

}  // namespace brave_ads
