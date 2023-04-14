/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/allow_notifications_permission_rule.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

class BatAdsAllowNotificationsPermissionRuleTest : public UnitTestBase {
 protected:
  AllowNotificationsPermissionRule permission_rule_;
};

TEST_F(BatAdsAllowNotificationsPermissionRuleTest, AllowAd) {
  // Arrange
  MockCanShowNotificationAds(ads_client_mock_, true);

  // Act

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow());
}

TEST_F(BatAdsAllowNotificationsPermissionRuleTest, DoNotAllowAd) {
  // Arrange
  MockCanShowNotificationAds(ads_client_mock_, false);

  // Act

  // Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow());
}

}  // namespace brave_ads
