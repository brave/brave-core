/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_type_test_util.h"
#include "brave/components/brave_ads/core/internal/ads_observer_mock.h"
#include "brave/components/brave_ads/core/internal/ads_observer_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_database_table_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_manager.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_test_util.h"
#include "brave/components/brave_ads/core/internal/reminders/reminders_constants.h"
#include "brave/components/brave_ads/core/internal/reminders/reminders_feature.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

void BuildAndSaveAdHistory(mojom::AdType mojom_ad_type, int clicked_count) {
  const std::vector<mojom::ConfirmationType> mojom_confirmation_types =
      test::BuildConfirmationTypeForCountAndIntersperseOtherTypes(
          mojom::ConfirmationType::kClicked, clicked_count);

  const AdHistoryList ad_history =
      test::BuildAdHistory(mojom_ad_type, mojom_confirmation_types,
                           /*should_generate_random_uuids=*/false);

  database::SaveAdHistory(ad_history);
}

}  // namespace

class BraveAdsRemindersTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    ads_observer_mock_ = test::MockAdsObserver();
  }

  raw_ptr<AdsObserverMock> ads_observer_mock_ = nullptr;  // Not owned.
};

TEST_F(BraveAdsRemindersTest,
       DoNotShowUserClickTheSameAdMultipleTimesReminderForAndroid) {
  // Arrange
  test::MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  BuildAndSaveAdHistory(
      mojom::AdType::kNotificationAd,
      /*clicked_count=*/kRemindUserIfClickingTheSameAdAfter.Get() - 1);

  const NotificationAdInfo ad =
      BuildNotificationAd(test::BuildCreativeNotificationAd(
          /*should_generate_random_uuids=*/false));

  // Act & Assert
  EXPECT_CALL(*ads_observer_mock_, OnRemindUser).Times(0);
  AdHistoryManager::GetInstance().Add(ad, mojom::ConfirmationType::kClicked);
  FastForwardClockBy(kMaybeShowReminderAfter);
}

TEST_F(BraveAdsRemindersTest,
       DoNotShowUserClickTheSameAdMultipleTimesReminderForIOS) {
  // Arrange
  test::MockPlatformHelper(platform_helper_mock_, PlatformType::kIOS);

  BuildAndSaveAdHistory(
      mojom::AdType::kNotificationAd,
      /*clicked_count=*/kRemindUserIfClickingTheSameAdAfter.Get() - 1);

  const NotificationAdInfo ad =
      BuildNotificationAd(test::BuildCreativeNotificationAd(
          /*should_generate_random_uuids=*/false));

  // Act & Assert
  EXPECT_CALL(*ads_observer_mock_, OnRemindUser).Times(0);
  AdHistoryManager::GetInstance().Add(ad, mojom::ConfirmationType::kClicked);
  FastForwardClockBy(kMaybeShowReminderAfter);
}

TEST_F(BraveAdsRemindersTest,
       ShowReminderWhenUserClicksTheSameAdMultipleTimesForWindows) {
  // Arrange
  test::MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  BuildAndSaveAdHistory(
      mojom::AdType::kNotificationAd,
      /*clicked_count=*/kRemindUserIfClickingTheSameAdAfter.Get() - 1);

  const NotificationAdInfo ad =
      BuildNotificationAd(test::BuildCreativeNotificationAd(
          /*should_generate_random_uuids=*/false));

  // Act & Assert
  EXPECT_CALL(*ads_observer_mock_,
              OnRemindUser(mojom::ReminderType::kClickedSameAdMultipleTimes));
  AdHistoryManager::GetInstance().Add(ad, mojom::ConfirmationType::kClicked);
  FastForwardClockBy(kMaybeShowReminderAfter);
}

TEST_F(BraveAdsRemindersTest,
       ShowReminderWhenUserClicksTheSameAdMultipleTimesForMacOs) {
  // Arrange
  test::MockPlatformHelper(platform_helper_mock_, PlatformType::kMacOS);

  BuildAndSaveAdHistory(
      mojom::AdType::kNotificationAd,
      /*clicked_count=*/kRemindUserIfClickingTheSameAdAfter.Get() - 1);

  const NotificationAdInfo ad =
      BuildNotificationAd(test::BuildCreativeNotificationAd(
          /*should_generate_random_uuids=*/false));

  // Act & Assert
  EXPECT_CALL(*ads_observer_mock_,
              OnRemindUser(mojom::ReminderType::kClickedSameAdMultipleTimes));
  AdHistoryManager::GetInstance().Add(ad, mojom::ConfirmationType::kClicked);
  FastForwardClockBy(kMaybeShowReminderAfter);
}

TEST_F(BraveAdsRemindersTest,
       ShowReminderWhenUserClicksTheSameAdMultipleTimesForLinux) {
  // Arrange
  test::MockPlatformHelper(platform_helper_mock_, PlatformType::kLinux);

  BuildAndSaveAdHistory(
      mojom::AdType::kNotificationAd,
      /*clicked_count=*/kRemindUserIfClickingTheSameAdAfter.Get() - 1);

  const NotificationAdInfo ad =
      BuildNotificationAd(test::BuildCreativeNotificationAd(
          /*should_generate_random_uuids=*/false));

  // Act & Assert
  EXPECT_CALL(*ads_observer_mock_,
              OnRemindUser(mojom::ReminderType::kClickedSameAdMultipleTimes));
  AdHistoryManager::GetInstance().Add(ad, mojom::ConfirmationType::kClicked);
  FastForwardClockBy(kMaybeShowReminderAfter);
}

TEST_F(BraveAdsRemindersTest,
       DoNotShowReminderIfUserDoesNotClickTheSameAdMultipleTimes) {
  // Arrange
  BuildAndSaveAdHistory(
      mojom::AdType::kNotificationAd,
      /*clicked_count=*/kRemindUserIfClickingTheSameAdAfter.Get() - 2);

  const NotificationAdInfo ad =
      BuildNotificationAd(test::BuildCreativeNotificationAd(
          /*should_generate_random_uuids=*/false));

  // Act & Assert
  EXPECT_CALL(*ads_observer_mock_, OnRemindUser).Times(0);
  AdHistoryManager::GetInstance().Add(ad, mojom::ConfirmationType::kClicked);
  FastForwardClockBy(kMaybeShowReminderAfter);
}

TEST_F(
    BraveAdsRemindersTest,
    DoNotShowReminderIfUserDoesNotClickTheSameAdMultipleTimesIfRemindersFeatureIsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kRemindersFeature);

  BuildAndSaveAdHistory(
      mojom::AdType::kNotificationAd,
      /*clicked_count=*/kRemindUserIfClickingTheSameAdAfter.Get() - 1);

  const NotificationAdInfo ad =
      BuildNotificationAd(test::BuildCreativeNotificationAd(
          /*should_generate_random_uuids=*/false));

  // Act & Assert
  EXPECT_CALL(*ads_observer_mock_, OnRemindUser).Times(0);
  AdHistoryManager::GetInstance().Add(ad, mojom::ConfirmationType::kClicked);
  FastForwardClockBy(kMaybeShowReminderAfter);
}

}  // namespace brave_ads
