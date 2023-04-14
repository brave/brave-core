/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/network_connection_permission_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rule_features.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

class BatAdsNetworkConnectionPermissionRuleTest : public UnitTestBase {};

TEST_F(BatAdsNetworkConnectionPermissionRuleTest, AllowAd) {
  // Arrange
  MockIsNetworkConnectionAvailable(ads_client_mock_, true);

  // Act
  NetworkConnectionPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNetworkConnectionPermissionRuleTest, DoNotAllowAd) {
  // Arrange
  MockIsNetworkConnectionAvailable(ads_client_mock_, false);

  // Act
  NetworkConnectionPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

TEST_F(BatAdsNetworkConnectionPermissionRuleTest,
       AllowAdIfPermissionRuleIsDisabled) {
  // Arrange
  base::FieldTrialParams params;
  params["should_only_serve_ads_with_valid_internet_connection"] = "false";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kPermissionRulesFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  MockIsNetworkConnectionAvailable(ads_client_mock_, false);

  // Act
  NetworkConnectionPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

}  // namespace brave_ads
