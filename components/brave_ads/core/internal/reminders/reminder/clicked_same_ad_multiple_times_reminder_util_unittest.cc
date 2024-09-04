/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/reminders/reminder/clicked_same_ad_multiple_times_reminder_util.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_type_test_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ads_observer_mock.h"
#include "brave/components/brave_ads/core/internal/ads_observer_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_test_util.h"
#include "brave/components/brave_ads/core/internal/reminders/reminders_feature.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsClickedSameAdMultipleTimesReminderUtilTest
    : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    ads_observer_mock_ = test::MockAdsObserver();
  }

  raw_ptr<AdsObserverMock> ads_observer_mock_ = nullptr;
};

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest, ShouldRemindUser) {
  // Act & Assert
  EXPECT_TRUE(ShouldRemindUser());
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       ShouldNotRemindUserWhenRemindersFeatureIsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kRemindersFeature);

  // Act & Assert
  EXPECT_FALSE(ShouldRemindUser());
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       ShouldNotRemindUser) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kRemindersFeature, {{"remind_user_if_clicking_the_same_ad_after", "0"}});

  // Act & Assert
  EXPECT_FALSE(ShouldRemindUser());
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       UserClickedTheSameAdMultipleTimes) {
  // Arrange
  const AdHistoryList ad_history = test::BuildAdHistoryForSamePlacement(
      mojom::AdType::kNotificationAd,
      test::BuildConfirmationTypeForCountAndIntersperseOtherTypes(
          mojom::ConfirmationType::kClicked,
          /*count=*/kRemindUserIfClickingTheSameAdAfter.Get()),
      /*should_generate_random_uuids=*/false);

  // Act & Assert
  EXPECT_TRUE(DidUserClickTheSameAdMultipleTimes(test::kCreativeInstanceId,
                                                 ad_history));
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       UserDidNotClickTheSameAdMultipleTimes) {
  // Arrange
  const AdHistoryList ad_history = test::BuildAdHistoryForSamePlacement(
      mojom::AdType::kNotificationAd,
      test::BuildConfirmationTypeForCountAndIntersperseOtherTypes(
          mojom::ConfirmationType::kClicked,
          /*count=*/kRemindUserIfClickingTheSameAdAfter.Get() - 1),
      /*should_generate_random_uuids=*/false);

  // Act & Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(test::kCreativeInstanceId,
                                                  ad_history));
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       UserClickedTheSameAdMultipleTimesConsecutively) {
  // Arrange
  const AdHistoryList ad_history = test::BuildAdHistoryForSamePlacement(
      mojom::AdType::kNotificationAd,
      test::BuildConfirmationTypeForCountAndIntersperseOtherTypes(
          mojom::ConfirmationType::kClicked,
          /*count=*/kRemindUserIfClickingTheSameAdAfter.Get() * 2),
      /*should_generate_random_uuids=*/false);

  // Act & Assert
  EXPECT_TRUE(DidUserClickTheSameAdMultipleTimes(test::kCreativeInstanceId,
                                                 ad_history));
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       UserDidNotClickTheSameAdMultipleTimesConsecutively) {
  // Arrange
  const AdHistoryList ad_history = test::BuildAdHistoryForSamePlacement(
      mojom::AdType::kNotificationAd,
      test::BuildConfirmationTypeForCountAndIntersperseOtherTypes(
          mojom::ConfirmationType::kClicked,
          /*count=*/(kRemindUserIfClickingTheSameAdAfter.Get() * 2) - 1),
      /*should_generate_random_uuids=*/false);

  // Act & Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(test::kCreativeInstanceId,
                                                  ad_history));
}

TEST_F(BraveAdsClickedSameAdMultipleTimesReminderUtilTest,
       UserClickedDifferentAdsMultipleTimes) {
  // Arrange
  const AdHistoryList ad_history = test::BuildAdHistoryForSamePlacement(
      mojom::AdType::kNotificationAd,
      test::BuildConfirmationTypeForCountAndIntersperseOtherTypes(
          mojom::ConfirmationType::kClicked,
          /*count=*/kRemindUserIfClickingTheSameAdAfter.Get()),
      /*should_generate_random_uuids=*/true);

  // Act & Assert
  EXPECT_FALSE(DidUserClickTheSameAdMultipleTimes(test::kCreativeInstanceId,
                                                  ad_history));
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
