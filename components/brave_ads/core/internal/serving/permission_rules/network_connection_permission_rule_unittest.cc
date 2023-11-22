/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rule_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNetworkConnectionPermissionRuleTest : public UnitTestBase {
};

TEST_F(BraveAdsNetworkConnectionPermissionRuleTest, ShouldAllow) {
  // Act & Assert
  EXPECT_TRUE(HasNetworkConnectionPermission());
}

TEST_F(BraveAdsNetworkConnectionPermissionRuleTest, ShouldNotAllow) {
  // Arrange
  MockIsNetworkConnectionAvailable(ads_client_mock_, false);

  // Act & Assert
  EXPECT_FALSE(HasNetworkConnectionPermission());
}

TEST_F(BraveAdsNetworkConnectionPermissionRuleTest,
       ShouldAllowIfPermissionRuleIsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kPermissionRulesFeature,
      {{"should_only_serve_ads_with_valid_internet_connection", "false"}});

  MockIsNetworkConnectionAvailable(ads_client_mock_, false);

  // Act & Assert
  EXPECT_TRUE(HasNetworkConnectionPermission());
}

}  // namespace brave_ads
