/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/reminder/reminder_manager.h"

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/browser/test/fake_ads_service_delegate.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_feature.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsReminderManagerTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};

  test::FakeAdsServiceDelegate delegate_;
  ReminderManager reminder_manager_{delegate_};
};

TEST_F(BraveAdsReminderManagerTest,
       MaybeShowDisplaysANotificationWithTitleAndBody) {
  // Act
  reminder_manager_.MaybeShow(mojom::ReminderType::kClickedSameAdMultipleTimes);

  // Assert
  EXPECT_THAT(delegate_.last_shown_notification_ad_title(),
              testing::Optional(testing::Not(testing::IsEmpty())));
  EXPECT_THAT(delegate_.last_shown_notification_ad_body(),
              testing::Optional(testing::Not(testing::IsEmpty())));
}

TEST_F(BraveAdsReminderManagerTest,
       IsShowingReminderReturnsTrueForKnownPlacementId) {
  // Arrange
  reminder_manager_.MaybeShow(mojom::ReminderType::kClickedSameAdMultipleTimes);
  ASSERT_TRUE(delegate_.last_shown_notification_ad_placement_id());
  const std::string placement_id =
      *delegate_.last_shown_notification_ad_placement_id();

  // Act & Assert
  EXPECT_TRUE(reminder_manager_.IsShowingReminder(placement_id));
}

TEST_F(BraveAdsReminderManagerTest,
       IsShowingReminderReturnsFalseForAnUnknownPlacementId) {
  // Act & Assert
  EXPECT_FALSE(reminder_manager_.IsShowingReminder("unknown_placement_id"));
}

TEST_F(BraveAdsReminderManagerTest,
       MaybeHandleClosedForgetsTheReminderAndReturnsTrue) {
  // Arrange
  reminder_manager_.MaybeShow(mojom::ReminderType::kClickedSameAdMultipleTimes);
  ASSERT_TRUE(delegate_.last_shown_notification_ad_placement_id());
  const std::string placement_id =
      *delegate_.last_shown_notification_ad_placement_id();

  // Act
  const bool was_reminder = reminder_manager_.MaybeHandleClosed(placement_id);

  // Assert
  EXPECT_TRUE(was_reminder);
  EXPECT_FALSE(reminder_manager_.IsShowingReminder(placement_id));
}

TEST_F(BraveAdsReminderManagerTest,
       MaybeHandleClosedReturnsFalseForAnUnknownPlacementId) {
  // Act & Assert
  EXPECT_FALSE(reminder_manager_.MaybeHandleClosed("unknown_placement_id"));
}

TEST_F(BraveAdsReminderManagerTest,
       MaybeHandleClickedOpensTheTargetUrlAndClosesTheNotification) {
  // Arrange
  reminder_manager_.MaybeShow(mojom::ReminderType::kClickedSameAdMultipleTimes);
  ASSERT_TRUE(delegate_.last_shown_notification_ad_placement_id());
  const std::string placement_id =
      *delegate_.last_shown_notification_ad_placement_id();

  // Act
  const bool was_reminder = reminder_manager_.MaybeHandleClicked(placement_id);

  // Assert
  ASSERT_TRUE(delegate_.last_opened_url());
  EXPECT_TRUE(was_reminder);
  EXPECT_TRUE(delegate_.last_opened_url()->is_valid());
  EXPECT_THAT(delegate_.closed_notification_ad_ids(),
              testing::ElementsAre(placement_id));
}

TEST_F(BraveAdsReminderManagerTest, ClosesTheNotificationWhenItTimesOut) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNotificationAdFeature, {{"notification_ad_timeout", "10s"}});

  reminder_manager_.MaybeShow(mojom::ReminderType::kClickedSameAdMultipleTimes);
  ASSERT_TRUE(delegate_.last_shown_notification_ad_placement_id());
  const std::string placement_id =
      *delegate_.last_shown_notification_ad_placement_id();

  // Act
  task_environment_.FastForwardBy(base::Seconds(10));

  // Assert
  EXPECT_THAT(delegate_.closed_notification_ad_ids(),
              testing::ElementsAre(placement_id));
}

TEST_F(BraveAdsReminderManagerTest,
       DoesNotCloseTheNotificationBeforeItTimesOut) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNotificationAdFeature, {{"notification_ad_timeout", "10s"}});

  reminder_manager_.MaybeShow(mojom::ReminderType::kClickedSameAdMultipleTimes);

  // Act
  task_environment_.FastForwardBy(base::Seconds(9));

  // Assert
  EXPECT_THAT(delegate_.closed_notification_ad_ids(), testing::IsEmpty());
}

TEST_F(BraveAdsReminderManagerTest,
       MaybeHandleClickedStopsTheTimeoutTimerSoItDoesNotCloseAgain) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNotificationAdFeature, {{"notification_ad_timeout", "10s"}});

  reminder_manager_.MaybeShow(mojom::ReminderType::kClickedSameAdMultipleTimes);
  ASSERT_TRUE(delegate_.last_shown_notification_ad_placement_id());
  const std::string placement_id =
      *delegate_.last_shown_notification_ad_placement_id();

  // Act
  reminder_manager_.MaybeHandleClicked(placement_id);
  task_environment_.FastForwardBy(base::Seconds(10));

  // Assert
  EXPECT_THAT(delegate_.closed_notification_ad_ids(),
              testing::ElementsAre(placement_id));
}

TEST_F(BraveAdsReminderManagerTest,
       MaybeHandleClickedReturnsFalseForAnUnknownPlacementId) {
  // Act & Assert
  EXPECT_FALSE(reminder_manager_.MaybeHandleClicked("unknown_placement_id"));
  EXPECT_FALSE(delegate_.last_opened_url());
}

}  // namespace brave_ads
