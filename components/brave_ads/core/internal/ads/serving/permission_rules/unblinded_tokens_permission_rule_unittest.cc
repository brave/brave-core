/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/unblinded_tokens_permission_rule.h"

#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsUnblindedTokensPermissionRuleTest : public UnitTestBase {
 protected:
  const UnblindedTokensPermissionRule permission_rule_;
};

TEST_F(BraveAdsUnblindedTokensPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCap) {
  // Arrange
  privacy::SetUnblindedTokens(/*count*/ 10);

  // Act

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsUnblindedTokensPermissionRuleTest,
       ShouldAllowIfUserHasNotJoinedBraveRewards) {
  // Arrange
  DisableBraveRewards();

  // Act

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsUnblindedTokensPermissionRuleTest,
       ShouldNotAllowIfNoUnblindedTokens) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsUnblindedTokensPermissionRuleTest, ShouldNotAllowIfExceedsCap) {
  // Arrange
  privacy::SetUnblindedTokens(/*count*/ 9);

  // Act

  // Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow().has_value());
}

}  // namespace brave_ads
