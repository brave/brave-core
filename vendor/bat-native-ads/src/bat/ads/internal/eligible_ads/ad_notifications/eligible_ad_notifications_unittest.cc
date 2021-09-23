/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/ad_notifications/eligible_ad_notifications.h"

#include <memory>
#include <string>

#include "base/guid.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_builder_unittest_util.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_info.h"
#include "bat/ads/internal/bundle/creative_ad_notification_unittest_util.h"
#include "bat/ads/internal/database/tables/creative_ad_notifications_database_table.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsEligibleAdNotificationsTest : public UnitTestBase {
 protected:
  BatAdsEligibleAdNotificationsTest()
      : database_table(
            std::make_unique<database::table::CreativeAdNotifications>()) {}

  ~BatAdsEligibleAdNotificationsTest() override = default;

  void RecordUserActivityEvents() {
    UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
    UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);
  }

  void Save(const CreativeAdNotificationList& creative_ad_notifications) {
    database_table->Save(creative_ad_notifications,
                         [](const bool success) { ASSERT_TRUE(success); });
  }

  std::unique_ptr<database::table::CreativeAdNotifications> database_table;
};

TEST_F(BatAdsEligibleAdNotificationsTest, GetAdsForParentChildSegment) {
  // Arrange
  CreativeAdNotificationList creative_ad_notifications;

  CreativeAdNotificationInfo creative_ad_notification_1 =
      GetCreativeAdNotificationForSegment("technology & computing");
  creative_ad_notifications.push_back(creative_ad_notification_1);

  CreativeAdNotificationInfo creative_ad_notification_2 =
      GetCreativeAdNotificationForSegment("technology & computing-software");
  creative_ad_notifications.push_back(creative_ad_notification_2);

  Save(creative_ad_notifications);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  ad_notifications::EligibleAds eligible_ads(&subdivision_targeting,
                                             &anti_targeting_resource);

  const CreativeAdNotificationList expected_creative_ad_notifications = {
      creative_ad_notification_2};

  eligible_ads.Get(
      ad_targeting::BuildUserModel({"technology & computing-software"}),
      [&expected_creative_ad_notifications](
          const bool success,
          const CreativeAdNotificationList& creative_ad_notifications) {
        EXPECT_EQ(expected_creative_ad_notifications,
                  creative_ad_notifications);
      });

  // Assert
}

TEST_F(BatAdsEligibleAdNotificationsTest, GetAdsForParentSegment) {
  // Arrange
  CreativeAdNotificationList creative_ad_notifications;

  CreativeAdNotificationInfo creative_ad_notification =
      GetCreativeAdNotificationForSegment("technology & computing");
  creative_ad_notifications.push_back(creative_ad_notification);

  Save(creative_ad_notifications);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  ad_notifications::EligibleAds eligible_ads(&subdivision_targeting,
                                             &anti_targeting_resource);

  const CreativeAdNotificationList expected_creative_ad_notifications = {
      creative_ad_notification};

  eligible_ads.Get(
      ad_targeting::BuildUserModel({"technology & computing-software"}),
      [&expected_creative_ad_notifications](
          const bool success,
          const CreativeAdNotificationList& creative_ad_notifications) {
        EXPECT_EQ(expected_creative_ad_notifications,
                  creative_ad_notifications);
      });

  // Assert
}

TEST_F(BatAdsEligibleAdNotificationsTest, GetAdsForUntargetedSegment) {
  // Arrange
  CreativeAdNotificationList creative_ad_notifications;

  CreativeAdNotificationInfo creative_ad_notification =
      GetCreativeAdNotificationForSegment("untargeted");
  creative_ad_notifications.push_back(creative_ad_notification);

  Save(creative_ad_notifications);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  ad_notifications::EligibleAds eligible_ads(&subdivision_targeting,
                                             &anti_targeting_resource);

  const CreativeAdNotificationList expected_creative_ad_notifications = {
      creative_ad_notification};

  eligible_ads.Get(
      ad_targeting::BuildUserModel({"finance-banking"}),
      [&expected_creative_ad_notifications](
          const bool success,
          const CreativeAdNotificationList& creative_ad_notifications) {
        EXPECT_EQ(expected_creative_ad_notifications,
                  creative_ad_notifications);
      });

  // Assert
}

TEST_F(BatAdsEligibleAdNotificationsTest, GetAdsForMultipleSegments) {
  // Arrange
  CreativeAdNotificationList creative_ad_notifications;

  CreativeAdNotificationInfo creative_ad_notification_1 =
      GetCreativeAdNotificationForSegment("technology & computing");
  creative_ad_notifications.push_back(creative_ad_notification_1);

  CreativeAdNotificationInfo creative_ad_notification_2 =
      GetCreativeAdNotificationForSegment("finance-banking");
  creative_ad_notifications.push_back(creative_ad_notification_2);

  CreativeAdNotificationInfo creative_ad_notification_3 =
      GetCreativeAdNotificationForSegment("food & drink");
  creative_ad_notifications.push_back(creative_ad_notification_3);

  Save(creative_ad_notifications);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  ad_notifications::EligibleAds eligible_ads(&subdivision_targeting,
                                             &anti_targeting_resource);

  const CreativeAdNotificationList expected_creative_ad_notifications = {
      creative_ad_notification_1, creative_ad_notification_2};

  eligible_ads.Get(
      ad_targeting::BuildUserModel({"technology & computing", "food & drink"}),
      [&expected_creative_ad_notifications](
          const bool success,
          const CreativeAdNotificationList& creative_ad_notifications) {
        EXPECT_EQ(expected_creative_ad_notifications,
                  creative_ad_notifications);
      });

  // Assert
}

TEST_F(BatAdsEligibleAdNotificationsTest, GetAdsForNoSegments) {
  // Arrange
  CreativeAdNotificationList creative_ad_notifications;

  CreativeAdNotificationInfo creative_ad_notification =
      GetCreativeAdNotificationForSegment("untargeted");
  creative_ad_notifications.push_back(creative_ad_notification);

  Save(creative_ad_notifications);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  ad_notifications::EligibleAds eligible_ads(&subdivision_targeting,
                                             &anti_targeting_resource);

  const CreativeAdNotificationList expected_creative_ad_notifications = {
      creative_ad_notification};

  eligible_ads.Get(
      {}, [&expected_creative_ad_notifications](
              const bool success,
              const CreativeAdNotificationList& creative_ad_notifications) {
        EXPECT_EQ(expected_creative_ad_notifications,
                  creative_ad_notifications);
      });

  // Assert
}

TEST_F(BatAdsEligibleAdNotificationsTest, GetAdsForUnmatchedSegments) {
  // Arrange
  CreativeAdNotificationList creative_ad_notifications;

  CreativeAdNotificationInfo creative_ad_notification =
      GetCreativeAdNotificationForSegment("technology & computing");
  creative_ad_notifications.push_back(creative_ad_notification);

  Save(creative_ad_notifications);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  ad_notifications::EligibleAds eligible_ads(&subdivision_targeting,
                                             &anti_targeting_resource);

  const CreativeAdNotificationList expected_creative_ad_notifications = {};

  eligible_ads.Get(
      ad_targeting::BuildUserModel({"UNMATCHED"}),
      [&expected_creative_ad_notifications](
          const bool success,
          const CreativeAdNotificationList& creative_ad_notifications) {
        EXPECT_EQ(expected_creative_ad_notifications,
                  creative_ad_notifications);
      });

  // Assert
}

TEST_F(BatAdsEligibleAdNotificationsTest, GetV2WithoutAds) {
  // Arrange
  const SegmentList interest_segments = {"interest-foo", "interest-bar"};
  const SegmentList purchase_intent_segments = {"intent-foo", "intent-bar"};
  const ad_targeting::UserModelInfo user_model =
      ad_targeting::BuildUserModel(interest_segments, purchase_intent_segments);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  ad_notifications::EligibleAds eligible_ads(&subdivision_targeting,
                                             &anti_targeting_resource);

  eligible_ads.GetV2(user_model,
                     [=](const bool was_allowed,
                         const absl::optional<CreativeAdNotificationInfo>& ad) {
                       EXPECT_EQ(absl::nullopt, ad);
                     });

  // Assert
}

TEST_F(BatAdsEligibleAdNotificationsTest, GetV2WithEmptySegments) {
  // Arrange
  CreativeAdNotificationList creative_ad_notifications;

  const CreativeAdNotificationInfo creative_ad_notification_1 =
      GetCreativeAdNotificationForSegment("foo");
  creative_ad_notifications.push_back(creative_ad_notification_1);

  const CreativeAdNotificationInfo creative_ad_notification_2 =
      GetCreativeAdNotificationForSegment("foo-bar");
  creative_ad_notifications.push_back(creative_ad_notification_2);

  Save(creative_ad_notifications);

  const SegmentList interest_segments = {};
  const SegmentList purchase_intent_segments = {};
  const ad_targeting::UserModelInfo user_model =
      ad_targeting::BuildUserModel(interest_segments, purchase_intent_segments);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  ad_notifications::EligibleAds eligible_ads(&subdivision_targeting,
                                             &anti_targeting_resource);

  const CreativeAdNotificationInfo expected_ad = creative_ad_notification_2;

  eligible_ads.GetV2(user_model,
                     [=](const bool was_allowed,
                         const absl::optional<CreativeAdNotificationInfo>& ad) {
                       EXPECT_TRUE(ad);
                     });

  // Assert
}

TEST_F(BatAdsEligibleAdNotificationsTest, GetV2) {
  // Arrange
  CreativeAdNotificationList creative_ad_notifications;

  const CreativeAdNotificationInfo creative_ad_notification_1 =
      GetCreativeAdNotificationForSegment("foo-bar1");
  creative_ad_notifications.push_back(creative_ad_notification_1);

  const CreativeAdNotificationInfo creative_ad_notification_2 =
      GetCreativeAdNotificationForSegment("foo-bar3");
  creative_ad_notifications.push_back(creative_ad_notification_2);

  Save(creative_ad_notifications);

  const SegmentList interest_segments = {"foo-bar3"};
  const SegmentList purchase_intent_segments = {"foo-bar1", "foo-bar2"};
  const ad_targeting::UserModelInfo user_model =
      ad_targeting::BuildUserModel(interest_segments, purchase_intent_segments);

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  ad_notifications::EligibleAds eligible_ads(&subdivision_targeting,
                                             &anti_targeting_resource);

  const CreativeAdNotificationInfo expected_ad = creative_ad_notification_2;

  eligible_ads.GetV2(user_model,
                     [=](const bool was_allowed,
                         const absl::optional<CreativeAdNotificationInfo>& ad) {
                       EXPECT_TRUE(ad);
                     });

  // Assert
}

}  // namespace ads
