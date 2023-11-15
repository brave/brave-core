/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules.h"

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsIssuersPermissionRuleTest : public UnitTestBase {
};

TEST_F(BraveAdsIssuersPermissionRuleTest, ShouldAllowForRewardsUser) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act & Assert
  EXPECT_TRUE(HasIssuersPermission());
}

TEST_F(BraveAdsIssuersPermissionRuleTest, ShouldAlwaysAllowForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_TRUE(HasIssuersPermission());
}

TEST_F(BraveAdsIssuersPermissionRuleTest, ShouldNotAllowIfNoIssuers) {
  // Act & Assert
  EXPECT_FALSE(HasIssuersPermission());
}

}  // namespace brave_ads
