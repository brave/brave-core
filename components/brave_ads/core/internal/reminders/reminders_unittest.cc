/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads_observer_mock.h"
#include "brave/components/brave_ads/core/internal/ads_observer_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_manager.h"
#include "brave/components/brave_ads/core/internal/reminders/reminders_feature.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

void AddAdHistoryForClickedEvent(const size_t count) {
  const NotificationAdInfo ad =
      BuildNotificationAd(test::BuildCreativeNotificationAd(
          /*should_generate_random_uuids=*/false));

  for (size_t i = 0; i < count; ++i) {
    AdHistoryManager::GetInstance().Add(ad, ConfirmationType::kClicked);
  }
}

}  // namespace

class BraveAdsRemindersTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    ads_observer_mock_ = test::AddAdsObserverMock();
  }

  raw_ptr<AdsObserverMock> ads_observer_mock_ = nullptr;
};

TEST_F(BraveAdsRemindersTest,
       ShowReminderWhenUserClicksTheSameAdMultipleTimes) {
  // Act & Assert
  EXPECT_CALL(*ads_observer_mock_,
              OnRemindUser(mojom::ReminderType::kClickedSameAdMultipleTimes));
  AddAdHistoryForClickedEvent(
      /*count=*/kRemindUserIfClickingTheSameAdAfter.Get());
  FastForwardClockBy(base::Seconds(1));
}

TEST_F(BraveAdsRemindersTest,
       DoNotShowReminderIfUserDoesNotClickTheSameAdMultipleTimes) {
  // Act & Assert
  EXPECT_CALL(*ads_observer_mock_, OnRemindUser).Times(0);
  AddAdHistoryForClickedEvent(
      /*count=*/kRemindUserIfClickingTheSameAdAfter.Get() - 1);
  FastForwardClockBy(base::Seconds(1));
}

TEST_F(
    BraveAdsRemindersTest,
    DoNotShowReminderIfUserDoesNotClickTheSameAdMultipleTimesWhenDisabledAddAdHistoryForClickedAdEvent) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kRemindersFeature);

  // Act & Assert
  EXPECT_CALL(*ads_observer_mock_, OnRemindUser).Times(0);
  AddAdHistoryForClickedEvent(
      /*count=*/kRemindUserIfClickingTheSameAdAfter.Get());
  FastForwardClockBy(base::Seconds(1));
}

}  // namespace brave_ads
