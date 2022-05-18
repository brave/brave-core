/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/allow_notifications_permission_rule.h"

#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsAllowNotificationsPermissionRuleTest : public UnitTestBase {
 protected:
  BatAdsAllowNotificationsPermissionRuleTest() = default;

  ~BatAdsAllowNotificationsPermissionRuleTest() override = default;
};

TEST_F(BatAdsAllowNotificationsPermissionRuleTest, AllowAd) {
  // Arrange
  MockShouldShowNotifications(ads_client_mock_, true);

  // Act
  AllowNotificationsPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsAllowNotificationsPermissionRuleTest, DoNotAllowAd) {
  // Arrange
  MockShouldShowNotifications(ads_client_mock_, false);

  // Act
  AllowNotificationsPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads
