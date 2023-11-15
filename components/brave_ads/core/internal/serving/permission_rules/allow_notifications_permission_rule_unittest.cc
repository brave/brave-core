/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAllowNotificationsPermissionRuleTest : public UnitTestBase {
};

TEST_F(BraveAdsAllowNotificationsPermissionRuleTest, ShouldAllow) {
  // Act & Assert
  EXPECT_TRUE(HasAllowNotificationsPermission());
}

TEST_F(BraveAdsAllowNotificationsPermissionRuleTest, ShouldNotAllow) {
  // Arrange
  MockCanShowNotificationAds(ads_client_mock_, false);

  // Act & Assert
  EXPECT_FALSE(HasAllowNotificationsPermission());
}

}  // namespace brave_ads
