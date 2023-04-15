/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/reminder/reminder.h"

#include <memory>
#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-shared.h"
#include "brave/components/brave_ads/core/history_item_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/reminder/reminder_features.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

using ::testing::_;

namespace {

void AddHistory(const size_t count) {
  const NotificationAdInfo ad = BuildNotificationAd(
      BuildCreativeNotificationAd(/*should_use_random_guids*/ false));

  for (size_t i = 0; i < count; i++) {
    HistoryManager::GetInstance()->Add(ad, ConfirmationType::kClicked);
  }
}

}  // namespace

class BatAdsReminderTest : public UnitTestBase {
  void SetUp() override {
    UnitTestBase::SetUp();

    reminder_ = std::make_unique<Reminder>();
  }

 protected:
  std::unique_ptr<Reminder> reminder_;
};

TEST_F(BatAdsReminderTest, ShowReminderWhenUserClicksTheSameAdMultipleTimes) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_,
              ShowReminder(mojom::ReminderType::kClickedSameAdMultipleTimes));

  // Act
  AddHistory(/*count*/ features::GetRemindUserIfClickingTheSameAdAfter());

  // Assert
}

TEST_F(BatAdsReminderTest,
       DoNotShowReminderIfUserDoesNotClickTheSameAdMultipleTimes) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, ShowReminder(_)).Times(0);

  // Act
  AddHistory(/*count*/ features::GetRemindUserIfClickingTheSameAdAfter() - 1);

  // Assert
}

TEST_F(BatAdsReminderTest,
       DoNotShowReminderIfUserDoesNotClickTheSameAdMultipleTimesWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(features::kReminder);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  EXPECT_CALL(*ads_client_mock_, ShowReminder(_)).Times(0);

  // Act
  AddHistory(/*count*/ features::GetRemindUserIfClickingTheSameAdAfter());

  // Assert
}

}  // namespace brave_ads
