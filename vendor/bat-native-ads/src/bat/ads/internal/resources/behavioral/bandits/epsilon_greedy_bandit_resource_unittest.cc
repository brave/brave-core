/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/bandits/epsilon_greedy_bandit_resource.h"

#include <memory>

#include "bat/ads/internal/database/tables/creative_ad_notifications_database_table.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace resource {

class BatAdsEpsilonGreedyBanditResourceTest : public UnitTestBase {
 protected:
  BatAdsEpsilonGreedyBanditResourceTest()
      : database_table_(
            std::make_unique<database::table::CreativeAdNotifications>()) {}

  ~BatAdsEpsilonGreedyBanditResourceTest() override = default;

  void Save(const CreativeAdNotificationList& creative_ad_notifications) {
    database_table_->Save(creative_ad_notifications, [](const Result result) {
      ASSERT_EQ(Result::SUCCESS, result);
    });
  }

  std::unique_ptr<database::table::CreativeAdNotifications> database_table_;
};

TEST_F(BatAdsEpsilonGreedyBanditResourceTest, LoadFromDatabase) {
  // Arrange
  CreativeAdNotificationList creative_ad_notifications;

  CreativeDaypartInfo daypart_info;
  CreativeAdNotificationInfo info;
  info.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info.start_at_timestamp = DistantPastAsTimestamp();
  info.end_at_timestamp = DistantFutureAsTimestamp();
  info.daily_cap = 1;
  info.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info.priority = 2;
  info.per_day = 3;
  info.total_max = 4;
  info.segment = "Technology & Computing-Software";
  info.dayparts.push_back(daypart_info);
  info.geo_targets = {"US"};
  info.target_url = "https://brave.com";
  info.title = "Test Ad Title";
  info.body = "Test Ad Body";
  info.ptr = 1.0;
  creative_ad_notifications.push_back(info);

  Save(creative_ad_notifications);

  // Act
  resource::EpsilonGreedyBandit resource;
  resource.LoadFromDatabase();

  // Assert
  const bool is_initialized = resource.IsInitialized();
  EXPECT_TRUE(is_initialized);
}

TEST_F(BatAdsEpsilonGreedyBanditResourceTest, LoadFromEmptyDatabase) {
  // Arrange

  // Act
  resource::EpsilonGreedyBandit resource;
  resource.LoadFromDatabase();

  // Assert
  const bool is_initialized = resource.IsInitialized();
  EXPECT_TRUE(is_initialized);
}

TEST_F(BatAdsEpsilonGreedyBanditResourceTest, DoNotLoadFromDatabase) {
  // Arrange

  // Act
  resource::EpsilonGreedyBandit resource;

  // Assert
  const bool is_initialized = resource.IsInitialized();
  EXPECT_FALSE(is_initialized);
}

}  // namespace resource
}  // namespace ads
