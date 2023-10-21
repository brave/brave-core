/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/reminder/reminder.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/reminder/reminder_feature.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

void AddHistory(const size_t count) {
  const NotificationAdInfo ad = BuildNotificationAd(
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true));

  for (size_t i = 0; i < count; ++i) {
    HistoryManager::GetInstance().Add(ad, ConfirmationType::kClicked);
  }
}

}  // namespace

class BraveAdsReminderTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    reminder_ = std::make_unique<Reminder>();
  }

  std::unique_ptr<Reminder> reminder_;
};

TEST_F(BraveAdsReminderTest, ShowReminderWhenUserClicksTheSameAdMultipleTimes) {
  // Act & Assert
  EXPECT_CALL(ads_client_mock_,
              ShowReminder(mojom::ReminderType::kClickedSameAdMultipleTimes));
  AddHistory(/*count=*/kRemindUserIfClickingTheSameAdAfter.Get());
  FastForwardClockBy(base::Seconds(1));
}

TEST_F(BraveAdsReminderTest,
       DoNotShowReminderIfUserDoesNotClickTheSameAdMultipleTimes) {
  // Act & Assert
  EXPECT_CALL(ads_client_mock_, ShowReminder).Times(0);
  AddHistory(/*count=*/kRemindUserIfClickingTheSameAdAfter.Get() - 1);
  FastForwardClockBy(base::Seconds(1));
}

TEST_F(BraveAdsReminderTest,
       DoNotShowReminderIfUserDoesNotClickTheSameAdMultipleTimesWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kReminderFeature);

  // Act & Assert
  EXPECT_CALL(ads_client_mock_, ShowReminder).Times(0);
  AddHistory(/*count=*/kRemindUserIfClickingTheSameAdAfter.Get());
  FastForwardClockBy(base::Seconds(1));
}

}  // namespace brave_ads
