/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/issuers_permission_rule.h"

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

class BatAdsIssuersPermissionRuleTest : public UnitTestBase {
 protected:
  IssuersPermissionRule permission_rule_;
};

TEST_F(BatAdsIssuersPermissionRuleTest, Issuers) {
  // Arrange
  BuildAndSetIssuers();

  // Act

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow());
}

TEST_F(BatAdsIssuersPermissionRuleTest, NoIssuers) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow());
}

}  // namespace brave_ads
