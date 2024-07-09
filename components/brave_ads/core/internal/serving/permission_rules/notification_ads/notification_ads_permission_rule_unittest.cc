/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/notification_ads/notification_ads_permission_rule.h"

#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNotificationAdsPermissionRuleTest : public test::TestBase {};

TEST_F(BraveAdsNotificationAdsPermissionRuleTest, ShouldAllow) {
  // Act & Assert
  EXPECT_TRUE(HasNotificationAdsPermission());
}

TEST_F(BraveAdsNotificationAdsPermissionRuleTest, ShouldNotAllow) {
  // Arrange
  test::MockCanShowNotificationAds(ads_client_mock_, false);

  // Act & Assert
  EXPECT_FALSE(HasNotificationAdsPermission());
}

}  // namespace brave_ads
